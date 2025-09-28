# PowerSequencer

PowerSequencer coordinates a group of Digital Loggers Web Power Switch units so that a single
operator can monitor load telemetry and toggle power to multiple AV racks from one place.
The project currently contains two pieces:

* **control-dashboard.html** – a standalone HTML dashboard that runs in any modern browser
  and issues REST commands to each rack controller.
* **webPowerSwitchAllOutlets** – firmware for an ESP8266/ESP32 bridge that exposes `/on`,
  `/off`, and `/status` endpoints while proxying commands to a Web Power Switch and
  aggregating telemetry.

## Features

* 2×2 responsive dashboard layout that surfaces each rack's state, status details, and
  per-strip current draw when the Digital Loggers API provides the information.
* Automatic refresh every five seconds so the UI reflects the latest state after manual or
  scripted changes.
* Embedded firmware support for caching strip current measurements, exposing total load, and
  reporting command success to callers of `/status`.

## Getting Started

### Control dashboard

1. Host `control-dashboard.html` on any static web server or open it directly in a browser.
2. Edit the `stations` array near the bottom of the file so each entry points at the base URL
   of the ESP controller that fronts a Web Power Switch.
3. The dashboard will poll each controller's `/status` endpoint every five seconds and render
   the response, including amperage readings if the backend makes them available.

### ESP bridge firmware

1. Open `webPowerSwitchAllOutlets` in the Arduino IDE or PlatformIO.
2. Update the Wi-Fi credentials and IP addresses at the top of the file to match your
   environment.
3. Flash the firmware to a supported ESP8266/ESP32 board. The sketch hosts HTTP endpoints for
   `/on`, `/off`, `/error`, and `/status` and forwards commands to the configured Web Power
   Switch devices using the Digital Loggers REST API.

Refer to the official Digital Loggers API reference for endpoint details and parameters:
https://www.digital-loggers.com/downloads/Product%20Manuals/Power%20Control/rest_api_reference.pdf

## Development notes

* The UI assumes the `/status` response returns JSON including `power`, `details`,
  `total_current`, and a `strips` array with `name`, `current`, and `current_available`
  properties for each Web Power Switch strip.
* Current telemetry is optional; when it is absent the dashboard gracefully displays an
  informative placeholder message.
* The firmware code relies on the ArduinoJson library and the ESP HTTP client stack that ships
  with the ESP8266/ESP32 Arduino cores.

