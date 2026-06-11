#ifndef SEQUENCE_ENGINE_H
#define SEQUENCE_ENGINE_H
#include "Types.h"
#include "Config.h"
#include <HTTPClient.h>

bool sendBatchCommand(const String& ip, const String& command) {
  HTTPClient http;
  http.setConnectTimeout(1000);
  http.setTimeout(5000);
  http.begin("http://" + ip + "/outlet?a=" + command);
  http.setAuthorization(DLI_USER, DLI_PASS);
  int code = http.GET();
  http.end();
  return code == 200;
}
#endif
