#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

enum class StripState : uint8_t { Off, On, Mixed, Error, Unknown };

struct OutletStatus {
  String name;
  bool state;
};

struct PowerStrip {
  String name;
  String ip;
  mutable String workingEndpoint;
  StripState state;
  float amps;
  bool online;
  String lastError;       // why the last poll failed (empty when online)
  String ampsDebug;       // endpoint + raw payload behind the amps reading
  bool ampsProbeLogged;   // probe results logged since the strip came online
  OutletStatus outlets[8];
};

enum class SystemState : uint8_t { Off, On, Mixed, Error, Sequencing, Updating };

#endif
