#ifndef STATUS_AGGREGATOR_H
#define STATUS_AGGREGATOR_H
#include "Types.h"
#include "Config.h"
#include "DiagLog.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

float extractAmps(String payload) {
  payload.trim();
  if (payload.length() == 0) return 0.0;

  // Try plain float first
  if (isdigit(payload[0]) || payload[0] == '-') {
    char* endptr;
    float val = strtof(payload.c_str(), &endptr);
    if (*endptr == '\0' || isspace(*endptr)) return val;
  }

  // Try parsing as JSON
  StaticJsonDocument<512> doc;
  if (!deserializeJson(doc, payload)) {
    if (doc.is<float>() || doc.is<int>()) return doc.as<float>();
    if (doc.is<JsonObject>()) {
      const char* keys[] = {"value", "amps", "current", "reading"};
      for (const char* k : keys) {
        if (doc.containsKey(k)) return doc[k].as<float>();
      }
    }
    if (doc.is<JsonArray>()) return doc[0].as<float>();
  }
  return 0.0;
}

// Human-readable HTTP result for log lines: library errors (timeouts,
// refused connections) come back as negative codes from HTTPClient.
String httpCodeStr(int code) {
  String out = "HTTP " + String(code);
  if (code < 0) out += " (" + HTTPClient::errorToString(code) + ")";
  else if (code == 401) out += " (auth rejected - check DLI_USER/DLI_PASS)";
  return out;
}

void pollStrip(PowerStrip& s) {
  bool wasOnline = s.online;
  String prevError = s.lastError;

  HTTPClient http;
  http.setConnectTimeout(700);
  http.setTimeout(1500);
  http.begin("http://" + s.ip + "/restapi/relay/outlets/");
  http.setAuthorization(DLI_USER, DLI_PASS);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      bool anyOn = false, anyOff = false;
      JsonArray arr = doc.as<JsonArray>();
      for (int i=0; i<8 && i<arr.size(); i++) {
        s.outlets[i].name = arr[i]["name"].as<String>();
        s.outlets[i].state = arr[i]["state"].as<bool>();
        if (s.outlets[i].state) anyOn = true; else anyOff = true;
      }
      s.state = (anyOn && anyOff) ? StripState::Mixed : (anyOn ? StripState::On : StripState::Off);
      s.online = true;
      s.lastError = "";
    } else {
      s.online = false;
      s.lastError = String("outlet list JSON error: ") + err.c_str() +
                    ", payload: \"" + payloadSnippet(payload) + "\"";
    }
  } else {
    s.online = false;
    s.lastError = "outlet list " + httpCodeStr(code);
  }
  http.end();

  // Log online/offline transitions and changes in the failure reason,
  // but not every failed poll (polls run every second).
  if (s.online && !wasOnline) {
    DiagLog.log("Rack '" + s.name + "' (" + s.ip + ") ONLINE");
    s.ampsProbeLogged = false;  // re-log amps probing after each reconnect
  } else if (!s.online && (wasOnline || s.lastError != prevError)) {
    DiagLog.log("Rack '" + s.name + "' (" + s.ip + ") OFFLINE: " + s.lastError);
  }

  // Unreachable strip: don't burn 4 more connect timeouts probing amps endpoints.
  if (!s.online) return;

  const char* endpoints[] = {"/restapi/relay/amps/", "/restapi/relay/current/", "/restapi/relay/status/", "/amps"};
  bool found = false;
  if (s.workingEndpoint.length() > 0) {
    http.begin("http://" + s.ip + s.workingEndpoint);
    http.setAuthorization(DLI_USER, DLI_PASS);
    int c = http.GET();
    if (c == 200) {
      String payload = http.getString();
      s.amps = extractAmps(payload);
      s.ampsDebug = s.workingEndpoint + " -> \"" + payloadSnippet(payload) + "\" = " + String(s.amps, 2) + " A";
      found = true;
    } else {
      DiagLog.log("Rack '" + s.name + "' amps endpoint " + s.workingEndpoint +
                  " stopped working (" + httpCodeStr(c) + "), re-probing");
      s.workingEndpoint = "";
    }
    http.end();
  }
  if (!found) {
    String probeResults;
    for (const char* ep : endpoints) {
      http.begin("http://" + s.ip + ep);
      http.setAuthorization(DLI_USER, DLI_PASS);
      int c = http.GET();
      if (c == 200) {
        String payload = http.getString();
        s.amps = extractAmps(payload);
        s.workingEndpoint = ep;
        s.ampsDebug = s.workingEndpoint + " -> \"" + payloadSnippet(payload) + "\" = " + String(s.amps, 2) + " A";
        found = true;
        DiagLog.log("Rack '" + s.name + "' amps endpoint found: " + s.ampsDebug);
        http.end();
        break;
      }
      probeResults += String(ep) + " -> " + httpCodeStr(c) + "; ";
      http.end();
    }
    if (!found) {
      s.amps = 0;
      s.ampsDebug = "no amps endpoint responded";
      if (!s.ampsProbeLogged) {
        DiagLog.log("Rack '" + s.name + "' no amps endpoint responded, current will read 0: " + probeResults);
      }
    }
    s.ampsProbeLogged = true;
  }
}
#endif
