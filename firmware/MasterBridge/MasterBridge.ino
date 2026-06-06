/**
 * PowerSequencer Master Bridge
 * Refactored Modular Firmware for ESP32 / Olimex / WT32-ETH01
 */

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <Preferences.h>

#include "Config.h"
#include "Types.h"
#include "Dashboard.h"
#include "StatusAggregator.h"
#include "SequenceEngine.h"

#if USE_ETHERNET
#  include <ETH.h>
#  define ETH_ADDR  1
#  define ETH_POWER_PIN 16
#  define ETH_MDC_PIN   23
#  define ETH_MDIO_PIN  18
#  define ETH_TYPE      ETH_PHY_LAN8720
#  define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#else
#  include <WiFi.h>
#endif

// --- System State ---
#define MAX_STRIPS 8
PowerStrip powerStrips[MAX_STRIPS];
uint8_t stripCount = 0;
WebServer server(80);
Preferences prefs;
SystemState globalState = SystemState::Off;

// --- Prototypes ---
void handleRoot();
void handleStatus();
void handleSaveConfig();
void updateStateIndicator();
bool applyCommandSequentially(const String& command);
SystemState calculateGlobalState();
void loadConfig();

#if USE_ETHERNET
bool eth_connected = false;
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_GOT_IP: eth_connected = true; break;
    case ARDUINO_EVENT_ETH_DISCONNECTED: eth_connected = false; break;
    default: break;
  }
}
#endif

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  loadConfig();

#if USE_ETHERNET
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  unsigned long start = millis();
  while (!eth_connected && millis() - start < 10000) { delay(500); Serial.print("."); }
#else
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
#endif

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/save_config", HTTP_POST, handleSaveConfig);
  server.on("/on", HTTP_GET, []() { applyCommandSequentially("ON"); server.send(200, "application/json", "{\"success\":true}"); });
  server.on("/off", HTTP_GET, []() { applyCommandSequentially("OFF"); server.send(200, "application/json", "{\"success\":true}"); });

  // Independent Rack Control
  server.on(UriBraces("/rack/{}/{}"), HTTP_GET, []() {
    int index = server.pathArg(0).toInt();
    String cmd = server.pathArg(1);
    if (index >= 0 && index < stripCount) {
      bool success = sendBatchCommand(powerStrips[index].ip, cmd.equalsIgnoreCase("on") ? "ON" : "OFF");
      server.send(success ? 200 : 502, "application/json", "{\"success\":" + String(success ? "true" : "false") + "}");
      pollStrip(powerStrips[index]);
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
      HTTPClient http;
      http.begin("http://" + powerStrips[index].ip + "/restapi/relay/outlets/" + String(oid) + "/name/");
      http.setAuthorization(DLI_USER, DLI_PASS);
      int code = http.PUT("{\"value\":\"" + newName + "\"}");
      http.end();
      server.send(code == 200 || code == 204 ? 200 : 502);
      pollStrip(powerStrips[index]);
    } else { server.send(400); }
  });

  server.begin();
  Serial.println("\nReady.");
}

void loop() {
  server.handleClient();
  static unsigned long lastPoll = 0;
  static uint8_t currentStrip = 0;
  if (stripCount > 0 && millis() - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = millis();
    if (globalState != SystemState::Sequencing) {
      pollStrip(powerStrips[currentStrip]);
      currentStrip = (currentStrip + 1) % stripCount;
      globalState = calculateGlobalState();
    }
  }
  updateStateIndicator();
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      applyCommandSequentially(globalState == SystemState::On ? "OFF" : "ON");
      while (digitalRead(BUTTON_PIN) == LOW) { server.handleClient(); delay(10); }
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
  DynamicJsonDocument doc(4096);
  const char* sysStrs[] = {"off", "on", "mixed", "error", "sequencing"};
  doc["power"] = sysStrs[(uint8_t)globalState];
  JsonArray strips = doc.createNestedArray("strips");
  float totalCurrent = 0;
  for (uint8_t i = 0; i < stripCount; i++) {
    JsonObject s = strips.createNestedObject();
    s["name"] = powerStrips[i].name; s["ip"] = powerStrips[i].ip;
    s["current"] = powerStrips[i].amps; s["current_available"] = powerStrips[i].online;
    if (powerStrips[i].online) totalCurrent += powerStrips[i].amps;
    JsonArray o_arr = s.createNestedArray("outlets");
    for (int j=0; j<8; j++) {
      JsonObject o = o_arr.createNestedObject();
      o["name"] = powerStrips[i].outlets[j].name;
      o["state"] = powerStrips[i].outlets[j].state;
    }
  }
  doc["total_current"] = totalCurrent;
  String res; serializeJson(doc, res);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", res);
}

void handleRoot() {
  server.sendHeader("Content-Encoding", "identity");
  server.send_P(200, "text/html", (const char*)INDEX_HTML, INDEX_HTML_SIZE);
}

bool applyCommandSequentially(const String& command) {
  globalState = SystemState::Sequencing;
  for (uint8_t i = 0; i < stripCount; i++) {
    sendBatchCommand(powerStrips[i].ip, command);
    if (i < stripCount - 1) {
      unsigned long start = millis();
      while(millis() - start < SEQUENCE_DELAY_MS) { server.handleClient(); updateStateIndicator(); delay(10); }
    }
  }
  for (uint8_t i = 0; i < stripCount; i++) pollStrip(powerStrips[i]);
  globalState = calculateGlobalState();
  return true;
}

void updateStateIndicator() {
  static unsigned long lastToggle = 0; static bool ledState = false;
  switch (globalState) {
    case SystemState::On: digitalWrite(STATUS_LED_PIN, HIGH); break;
    case SystemState::Off: digitalWrite(STATUS_LED_PIN, LOW); break;
    case SystemState::Sequencing: if (millis() - lastToggle >= 250) { lastToggle = millis(); ledState = !ledState; digitalWrite(STATUS_LED_PIN, ledState); } break;
    case SystemState::Error: case SystemState::Mixed: if (millis() - lastToggle >= 100) { lastToggle = millis(); ledState = !ledState; digitalWrite(STATUS_LED_PIN, ledState); } break;
    default: digitalWrite(STATUS_LED_PIN, LOW);
  }
}
