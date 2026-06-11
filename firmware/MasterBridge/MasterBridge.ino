/**
 * PowerSequencer Master Bridge
 * Refactored Modular Firmware for ESP32 / Olimex / WT32-ETH01
 *
 * All strip HTTP I/O (polling + commands) runs on a background FreeRTOS
 * task so loop() stays free for the web server, buttons and LEDs.
 */

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>

#include "Config.h"
#include "Types.h"
#include "DiagLog.h"
#include "Dashboard.h"
#include "StatusAggregator.h"
#include "SequenceEngine.h"

#if USE_ETHERNET
#  include <ETH.h>
#  if ETH_BOARD == ETH_BOARD_ESP32_P4_ETH
     // Waveshare ESP32-P4-ETH: IP101 PHY, RMII, external 50MHz REF_CLK on GPIO50
#    define ETH_ADDR      1
#    define ETH_MDC_PIN   31
#    define ETH_MDIO_PIN  52
#    define ETH_POWER_PIN 51
#    define ETH_TYPE      ETH_PHY_IP101
#    define ETH_CLK_MODE  ((eth_clock_mode_t)EMAC_CLK_EXT_IN)
#  else
     // WT32-ETH01 / Olimex ESP32-POE: LAN8720 PHY, RMII, REF_CLK on GPIO0
#    define ETH_ADDR      1
#    define ETH_POWER_PIN 16
#    define ETH_MDC_PIN   23
#    define ETH_MDIO_PIN  18
#    define ETH_TYPE      ETH_PHY_LAN8720
#    define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#  endif
#else
#  include <WiFi.h>
#endif

// --- System State ---
#define MAX_STRIPS 8
PowerStrip powerStrips[MAX_STRIPS];
uint8_t stripCount = 0;
WebServer server(80);
Preferences prefs;
volatile SystemState globalState = SystemState::Off;
const char* SYS_STATE_NAMES[] = {"off", "on", "mixed", "error", "sequencing", "updating"};
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- Async command processing ---
// Web handlers and buttons enqueue commands; the network task executes them.
enum class CmdType : uint8_t { SequenceAll, SingleRack };
struct Command {
  CmdType type;
  uint8_t rackIndex;
  bool turnOn;
};
QueueHandle_t cmdQueue;
SemaphoreHandle_t stateMutex;  // guards powerStrips[] contents (Strings)

// Set while an OTA flash is in progress; pauses strip I/O and the LED/web
// loop so the upload gets the CPU and flash bus to itself.
volatile bool otaInProgress = false;

// --- Prototypes ---
void handleRoot();
void handleStatus();
void handleSaveConfig();
void handleButtons();
void updateStateIndicator();
void enqueueCommand(const Command& cmd);
void applyGlobalState(SystemState next);
SystemState calculateGlobalState();
void loadConfig();
void networkTask(void* arg);

#if USE_ETHERNET
bool eth_connected = false;
void WiFiEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_GOT_IP: eth_connected = true; break;
    case ARDUINO_EVENT_ETH_DISCONNECTED: eth_connected = false; break;
    default: break;
  }
}
#endif

// Why did the device boot? An unexpected reset wipes status and the log,
// so record it as the first log entry.
const char* resetReasonStr() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_SW:        return "software restart";
    case ESP_RST_PANIC:     return "crash (panic)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "watchdog reset";
    case ESP_RST_BROWNOUT:  return "brownout (power dip)";
    default:                return "other";
  }
}

void setup() {
  Serial.begin(115200);
  DiagLog.begin();
  DiagLog.log(String("Boot, reset reason: ") + resetReasonStr());
  pinMode(MASTER_BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < sizeof(RACK_BUTTON_PINS); i++) {
    pinMode(RACK_BUTTON_PINS[i], INPUT_PULLUP);
  }

  pixels.begin();
  pixels.setBrightness(50);
  pixels.show();

  stateMutex = xSemaphoreCreateMutex();
  cmdQueue = xQueueCreate(8, sizeof(Command));

  loadConfig();

#if USE_ETHERNET
  Network.onEvent(WiFiEvent);
  ETH.setHostname(OTA_HOSTNAME);
#if USE_STATIC_IP
  ETH.config(IPAddress(STATIC_IP), IPAddress(STATIC_GATEWAY), IPAddress(STATIC_SUBNET), IPAddress(STATIC_DNS));
#endif
  // arduino-esp32 3.x signature: (type, phy_addr, mdc, mdio, power, clock_mode)
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
  unsigned long start = millis();
  while (!eth_connected && millis() - start < 10000) { delay(500); Serial.print("."); }
  DiagLog.log(eth_connected ? "Ethernet up, IP: " + ETH.localIP().toString()
                            : "Ethernet NOT connected after 10s");
#else
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
#if USE_STATIC_IP
  WiFi.config(IPAddress(STATIC_IP), IPAddress(STATIC_GATEWAY), IPAddress(STATIC_SUBNET), IPAddress(STATIC_DNS));
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  DiagLog.log("WiFi up, IP: " + WiFi.localIP().toString());
#endif

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    globalState = SystemState::Updating;
    DiagLog.log("OTA update started");
  });
  ArduinoOTA.onEnd([]() { otaInProgress = false; });
  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    DiagLog.log("OTA update FAILED, error " + String((int)error));
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    applyGlobalState(calculateGlobalState());
    xSemaphoreGive(stateMutex);
  });
  ArduinoOTA.begin();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/log", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", DiagLog.dump());
  });
  server.on("/save_config", HTTP_POST, handleSaveConfig);
  server.on("/on", HTTP_GET, []() { enqueueCommand({CmdType::SequenceAll, 0, true}); server.send(200, "application/json", "{\"success\":true}"); });
  server.on("/off", HTTP_GET, []() { enqueueCommand({CmdType::SequenceAll, 0, false}); server.send(200, "application/json", "{\"success\":true}"); });

  // Independent Rack Control
  server.on(UriBraces("/rack/{}/{}"), HTTP_GET, []() {
    int index = server.pathArg(0).toInt();
    String cmd = server.pathArg(1);
    if (index >= 0 && index < stripCount) {
      enqueueCommand({CmdType::SingleRack, (uint8_t)index, cmd.equalsIgnoreCase("on")});
      server.send(200, "application/json", "{\"success\":true}");
    } else { server.send(404); }
  });

  // Outlet Renaming
  server.on(UriBraces("/rack/{}/outlet/{}/rename"), HTTP_POST, []() {
    int index = server.pathArg(0).toInt();
    int oid = server.pathArg(1).toInt();
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    String newName = doc["name"].as<String>();
    if (index >= 0 && index < stripCount && newName.length() > 0) {
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      String ip = powerStrips[index].ip;
      xSemaphoreGive(stateMutex);
      HTTPClient http;
      http.setConnectTimeout(1000);
      http.begin("http://" + ip + "/restapi/relay/outlets/" + String(oid) + "/name/");
      http.setAuthorization(DLI_USER, DLI_PASS);
      int code = http.PUT("{\"value\":\"" + newName + "\"}");
      http.end();
      if (code == 200 || code == 204) {
        // Update locally; the periodic poll will confirm.
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        if (oid >= 0 && oid < 8) powerStrips[index].outlets[oid].name = newName;
        xSemaphoreGive(stateMutex);
        server.send(200);
      } else server.send(502);
    } else { server.send(400); }
  });

  server.begin();

  xTaskCreatePinnedToCore(networkTask, "net", 12288, nullptr, 1, nullptr, 0);

  Serial.println("\nReady.");
}

void loop() {
  ArduinoOTA.handle();
  if (otaInProgress) {
    updateStateIndicator();
    return;
  }
  server.handleClient();
  handleButtons();
  updateStateIndicator();
}

void enqueueCommand(const Command& cmd) {
  if (cmd.type == CmdType::SequenceAll) {
    DiagLog.log(String("Command queued: sequence all ") + (cmd.turnOn ? "ON" : "OFF"));
    // Immediate UI feedback; the task recalculates when done.
    globalState = SystemState::Sequencing;
  } else {
    DiagLog.log("Command queued: rack " + String(cmd.rackIndex) + (cmd.turnOn ? " ON" : " OFF"));
  }
  xQueueSend(cmdQueue, &cmd, 0);
}

// Caller must hold stateMutex. Logs transitions, with the per-rack failure
// reasons when entering the error state, so /log explains every flip.
void applyGlobalState(SystemState next) {
  if (next == globalState) return;
  String msg = String("Global state: ") + SYS_STATE_NAMES[(uint8_t)globalState] +
               " -> " + SYS_STATE_NAMES[(uint8_t)next];
  if (next == SystemState::Error) {
    for (uint8_t i = 0; i < stripCount; i++) {
      if (!powerStrips[i].online) {
        msg += " | '" + powerStrips[i].name + "' offline: " + powerStrips[i].lastError;
      }
    }
  }
  globalState = next;
  DiagLog.log(msg);
}

// Reads a strip's address fields, polls it without holding the lock,
// then publishes the results.
void pollStripSafe(uint8_t i) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  PowerStrip tmp = powerStrips[i];
  xSemaphoreGive(stateMutex);

  pollStrip(tmp);

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  powerStrips[i].workingEndpoint = tmp.workingEndpoint;
  powerStrips[i].state = tmp.state;
  powerStrips[i].amps = tmp.amps;
  powerStrips[i].online = tmp.online;
  powerStrips[i].lastError = tmp.lastError;
  powerStrips[i].ampsDebug = tmp.ampsDebug;
  powerStrips[i].ampsProbeLogged = tmp.ampsProbeLogged;
  for (int j = 0; j < 8; j++) powerStrips[i].outlets[j] = tmp.outlets[j];
  // Keep showing Sequencing while commands are still queued.
  if (uxQueueMessagesWaiting(cmdQueue) == 0) applyGlobalState(calculateGlobalState());
  xSemaphoreGive(stateMutex);
}

String stripIp(uint8_t i) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  String ip = powerStrips[i].ip;
  xSemaphoreGive(stateMutex);
  return ip;
}

void networkTask(void* arg) {
  uint8_t currentStrip = 0;
  uint32_t lastPoll = 0;
  Command cmd;
  for (;;) {
    if (otaInProgress) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }
    if (xQueueReceive(cmdQueue, &cmd, pdMS_TO_TICKS(50)) == pdTRUE) {
      if (cmd.type == CmdType::SequenceAll) {
        globalState = SystemState::Sequencing;
        for (uint8_t i = 0; i < stripCount; i++) {
          sendBatchCommand(stripIp(i), cmd.turnOn ? "ON" : "OFF");
          if (i < stripCount - 1) vTaskDelay(pdMS_TO_TICKS(SEQUENCE_DELAY_MS));
        }
        for (uint8_t i = 0; i < stripCount; i++) pollStripSafe(i);
      } else {
        sendBatchCommand(stripIp(cmd.rackIndex), cmd.turnOn ? "ON" : "OFF");
        pollStripSafe(cmd.rackIndex);
      }
    } else if (stripCount > 0 && millis() - lastPoll >= POLL_INTERVAL_MS) {
      lastPoll = millis();
      pollStripSafe(currentStrip);
      currentStrip = (currentStrip + 1) % stripCount;
    }
  }
}

// Non-blocking edge-triggered buttons with 50ms debounce.
void handleButtons() {
  uint32_t now = millis();

  static bool masterPressed = false;
  static uint32_t masterChanged = 0;
  bool raw = digitalRead(MASTER_BUTTON_PIN) == LOW;
  if (raw != masterPressed && now - masterChanged > 50) {
    masterPressed = raw;
    masterChanged = now;
    if (masterPressed && globalState != SystemState::Sequencing) {
      enqueueCommand({CmdType::SequenceAll, 0, globalState != SystemState::On});
    }
  }

  static bool rackPressed[sizeof(RACK_BUTTON_PINS)] = {false};
  static uint32_t rackChanged[sizeof(RACK_BUTTON_PINS)] = {0};
  for (uint8_t i = 0; i < stripCount && i < sizeof(RACK_BUTTON_PINS); i++) {
    bool r = digitalRead(RACK_BUTTON_PINS[i]) == LOW;
    if (r != rackPressed[i] && now - rackChanged[i] > 50) {
      rackPressed[i] = r;
      rackChanged[i] = now;
      if (r) {
        enqueueCommand({CmdType::SingleRack, i, powerStrips[i].state != StripState::On});
      }
    }
  }
}

void loadConfig() {
  prefs.begin("sequencer", false);
  String json = prefs.getString("config", "");
  if (json.length() > 0) {
    StaticJsonDocument<2048> doc;
    if (!deserializeJson(doc, json)) {
      JsonArray arr = doc.as<JsonArray>();
      stripCount = 0;
      for (JsonVariant v : arr) {
        if (stripCount >= MAX_STRIPS) break;
        powerStrips[stripCount].name = v["name"].as<String>();
        powerStrips[stripCount].ip = v["ip"].as<String>();
        stripCount++;
      }
    }
  }
  prefs.end();
  if (stripCount == 0) {
    powerStrips[0] = {"Rack 1", "192.168.0.101", "", StripState::Unknown, 0, false};
    stripCount = 1;
  }
}

void handleSaveConfig() {
  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "Invalid JSON"); return; }
  prefs.begin("sequencer", false);
  prefs.putString("config", server.arg("plain"));
  prefs.end();
  server.send(200);
  delay(500); ESP.restart();
}

// Caller must hold stateMutex (or be the only writer).
SystemState calculateGlobalState() {
  bool anyOn = false, anyOff = false, anyError = false;
  for (uint8_t i = 0; i < stripCount; i++) {
    if (!powerStrips[i].online) anyError = true;
    else if (powerStrips[i].state == StripState::On) anyOn = true;
    else if (powerStrips[i].state == StripState::Off) anyOff = true;
    else if (powerStrips[i].state == StripState::Mixed) { anyOn = true; anyOff = true; }
  }
  if (anyError) return SystemState::Error;
  if (anyOn && anyOff) return SystemState::Mixed;
  if (anyOn) return SystemState::On;
  return SystemState::Off;
}

void handleStatus() {
  DynamicJsonDocument doc(6144);
  doc["power"] = SYS_STATE_NAMES[(uint8_t)globalState];
  JsonArray strips = doc.createNestedArray("strips");
  float totalCurrent = 0;
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  for (uint8_t i = 0; i < stripCount; i++) {
    JsonObject s = strips.createNestedObject();
    s["name"] = powerStrips[i].name; s["ip"] = powerStrips[i].ip;
    s["current"] = powerStrips[i].amps; s["current_available"] = powerStrips[i].online;
    s["last_error"] = powerStrips[i].lastError;
    s["amps_debug"] = powerStrips[i].ampsDebug;
    if (powerStrips[i].online) totalCurrent += powerStrips[i].amps;
    JsonArray o_arr = s.createNestedArray("outlets");
    for (int j=0; j<8; j++) {
      JsonObject o = o_arr.createNestedObject();
      o["name"] = powerStrips[i].outlets[j].name;
      o["state"] = powerStrips[i].outlets[j].state;
    }
  }
  xSemaphoreGive(stateMutex);
  doc["total_current"] = totalCurrent;
  String res; serializeJson(doc, res);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", res);
}

void handleRoot() {
  server.sendHeader("Content-Encoding", "identity");
  server.send_P(200, "text/html", (const char*)INDEX_HTML, INDEX_HTML_SIZE);
}

void updateStateIndicator() {
  static unsigned long lastToggle = 0;
  static bool flashState = false;
  if (millis() - lastToggle >= 300) {
    lastToggle = millis();
    flashState = !flashState;
  }

  uint32_t colors[NUM_LEDS];
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (i >= stripCount) {
      colors[i] = 0;
      continue;
    }

    if (globalState == SystemState::Updating) {
      colors[i] = flashState ? pixels.Color(255, 0, 255) : 0; // Flashing Purple
    } else if (globalState == SystemState::Sequencing) {
      colors[i] = flashState ? pixels.Color(0, 0, 255) : 0; // Flashing Blue
    } else if (!powerStrips[i].online) {
      colors[i] = flashState ? pixels.Color(255, 0, 0) : 0; // Flashing Red
    } else {
      switch (powerStrips[i].state) {
        case StripState::On:    colors[i] = pixels.Color(0, 255, 0); break;
        case StripState::Off:   colors[i] = pixels.Color(20, 0, 0);  break; // Dim red
        case StripState::Mixed: colors[i] = pixels.Color(255, 100, 0); break; // Orange
        default:                colors[i] = pixels.Color(10, 10, 10); break;
      }
    }
  }

  // pixels.show() is slow and interrupt-sensitive; only push when changed.
  static uint32_t prevColors[NUM_LEDS];
  static bool firstShow = true;
  if (firstShow || memcmp(colors, prevColors, sizeof(colors)) != 0) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) pixels.setPixelColor(i, colors[i]);
    pixels.show();
    memcpy(prevColors, colors, sizeof(colors));
    firstShow = false;
  }
}
