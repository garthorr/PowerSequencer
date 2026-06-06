#ifndef STATUS_AGGREGATOR_H
#define STATUS_AGGREGATOR_H
#include "Types.h"
#include "Config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

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

  const char* endpoints[] = {"/restapi/relay/amps/", "/restapi/relay/current/", "/amps"};
  bool found = false;
  if (s.workingEndpoint.length() > 0) {
    http.begin("http://" + s.ip + s.workingEndpoint);
    http.setAuthorization(DLI_USER, DLI_PASS);
    if (http.GET() == 200) { s.amps = http.getString().toFloat(); found = true; }
    http.end();
  }
  if (!found) {
    for (const char* ep : endpoints) {
      http.begin("http://" + s.ip + ep);
      http.setAuthorization(DLI_USER, DLI_PASS);
      if (http.GET() == 200) { s.amps = http.getString().toFloat(); s.workingEndpoint = ep; found = true; http.end(); break; }
      http.end();
    }
  }
}
#endif
