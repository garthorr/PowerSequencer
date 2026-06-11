#ifndef DIAG_LOG_H
#define DIAG_LOG_H

#include <Arduino.h>

// Ring buffer of recent diagnostic events, mirrored to Serial and served
// at /log so failures can be inspected without a serial cable. RAM-only:
// the log starts fresh on every boot.
class DiagLogClass {
public:
  void begin() { mutex = xSemaphoreCreateMutex(); }

  void log(const String& msg) {
    uint32_t s = millis() / 1000;
    char ts[16];
    snprintf(ts, sizeof(ts), "[%lu:%02lu:%02lu] ", (unsigned long)(s / 3600),
             (unsigned long)(s / 60 % 60), (unsigned long)(s % 60));
    String line = String(ts) + msg;
    Serial.println(line);
    if (!mutex) return;
    xSemaphoreTake(mutex, portMAX_DELAY);
    entries[(head + count) % CAPACITY] = line;
    if (count < CAPACITY) count++;
    else head = (head + 1) % CAPACITY;
    xSemaphoreGive(mutex);
  }

  String dump() {
    String out;
    out.reserve(2048);
    if (!mutex) return out;
    xSemaphoreTake(mutex, portMAX_DELAY);
    for (uint16_t i = 0; i < count; i++) {
      out += entries[(head + i) % CAPACITY];
      out += '\n';
    }
    xSemaphoreGive(mutex);
    return out;
  }

private:
  static const uint16_t CAPACITY = 60;
  String entries[CAPACITY];
  uint16_t head = 0, count = 0;
  SemaphoreHandle_t mutex = nullptr;
};

DiagLogClass DiagLog;

// First chars of an HTTP payload, flattened to one line, for log messages.
String payloadSnippet(const String& payload, unsigned int maxLen = 60) {
  String out = payload;
  out.trim();
  out.replace("\r", " ");
  out.replace("\n", " ");
  if (out.length() > maxLen) out = out.substring(0, maxLen) + "...";
  return out;
}

#endif
