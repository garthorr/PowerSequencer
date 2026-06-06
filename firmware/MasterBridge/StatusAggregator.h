#ifndef STATUS_AGGREGATOR_H
#define STATUS_AGGREGATOR_H
#include "Types.h"
#include "Config.h"
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

void pollStrip(PowerStrip& s) {
  HTTPClient http;
  http.setTimeout(2000);
  http.begin("http://" + s.ip + "/restapi/relay/outlets/");
  http.setAuthorization(DLI_USER, DLI_PASS);
  if (http.GET() == 200) {
    StaticJsonDocument<2048> doc;
    if (!deserializeJson(doc, http.getString())) {
      bool anyOn = false, anyOff = false;
      JsonArray arr = doc.as<JsonArray>();
      for (int i=0; i<8 && i<arr.size(); i++) {
        s.outlets[i].name = arr[i]["name"].as<String>();
        s.outlets[i].state = arr[i]["state"].as<bool>();
        if (s.outlets[i].state) anyOn = true; else anyOff = true;
      }
      s.state = (anyOn && anyOff) ? StripState::Mixed : (anyOn ? StripState::On : StripState::Off);
      s.online = true;
    } else s.online = false;
  } else s.online = false;
  http.end();

  const char* endpoints[] = {"/restapi/relay/amps/", "/restapi/relay/current/", "/restapi/relay/status/", "/amps"};
  bool found = false;
  if (s.workingEndpoint.length() > 0) {
    http.begin("http://" + s.ip + s.workingEndpoint);
    http.setAuthorization(DLI_USER, DLI_PASS);
    if (http.GET() == 200) {
      s.amps = extractAmps(http.getString());
      found = true;
    }
    http.end();
  }
  if (!found) {
    for (const char* ep : endpoints) {
      http.begin("http://" + s.ip + ep);
      http.setAuthorization(DLI_USER, DLI_PASS);
      if (http.GET() == 200) {
        float val = extractAmps(http.getString());
        s.amps = val;
        s.workingEndpoint = ep;
        found = true;
        http.end();
        break;
      }
      http.end();
    }
  }
}
#endif
