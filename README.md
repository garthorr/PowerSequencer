# PowerSequencer Master Bridge

PowerSequencer is a professional-grade master control bridge for Digital Loggers Web Power Switch units. It coordinates multiple AV racks, providing synchronized power sequencing and detailed telemetry monitoring from a single interface.

This project is built on a **modular architecture** designed for high stability and industrial deployment on **ESP32**, **Olimex ESP32-POE**, or **Waveshare ESP32-P4-ETH** hardware.

## Features

*   **Sequential Power Control**: Automatically staggers power commands across racks with a 1.5s delay to manage inrush current safely.
*   **Deep Telemetry**: Real-time status for the whole system, individual racks, and **individual outlets** (including names and states).
*   **Wired Reliability**: Native support for LAN8720 (WT32-ETH01 / Olimex) and IP101 (ESP32-P4-ETH) PHYs for mission-critical wired networking.
*   **Web-Based Configuration**: Manage rack names, IP addresses, and even GPIO pin assignments directly through the dashboard. Settings persist in non-volatile memory.
*   **Hardware Integration**: Physical master and per-rack toggle buttons, plus real-time NeoPixel status LEDs.
*   **Diagnostic Logging**: A ring buffer of recent events (offline reasons, HTTP errors, state changes, boot reset reason) served at `/log` — no serial cable needed.
*   **OTA Updates**: Reflash over the LAN from the Arduino IDE / arduino-cli (shows up as a Network Port).
*   **Production Ready**: Strip polling and commands run on a background task, so the web UI, buttons, and LEDs stay responsive even when units are offline.

---

## Hardware Setup

The bridge is designed to be a reliable hardware appliance.

### Buttons
*   **Master Button**: One press sequences **all racks** ON or OFF (toggles based on the current overall state).
*   **Rack Buttons**: Each button toggles its individual rack.
*   All buttons are momentary switches wired between the GPIO and **GND** (internal pullups, no external resistors needed).

### Status LEDs (NeoPixel / WS281x)
One pixel per rack, data line on a single GPIO. The color order and data rate are set by `LED_TYPE` in `Config.h` — the default `NEO_RGB + NEO_KHZ400` matches Adafruit 5mm/8mm thru-hole NeoPixels; use `NEO_GRB + NEO_KHZ800` for most SMD strips/pixels.
*   **Green**: Rack is ON.
*   **Dim Red**: Rack is OFF.
*   **Orange**: Mixed outlet states.
*   **Flashing Blue**: Sequencing in progress.
*   **Flashing Red**: Rack is Offline.
*   **Flashing Purple**: OTA update in progress.

### Default Pinout (WT32-ETH01 / Olimex / classic ESP32)
*   **Master Button**: IO12
*   **Rack Buttons** (racks 1-7): GPIOs 35, 36, 39, 15, 4, 32, 33
*   **LED Data**: IO14

### Default Pinout (Waveshare ESP32-P4-ETH)
The classic-ESP32 pins above don't exist on the P4 header, so this board uses its own map (selected automatically by `ETH_BOARD` in `Config.h`):
*   **Master Button**: GPIO15
*   **Rack Buttons** (racks 1-7): GPIOs 16, 17, 18, 19, 20, 21, 22
*   **LED Data**: GPIO14

All pin assignments are just firmware defaults — they can be changed at runtime from the dashboard under **Settings → Hardware Pins** (stored in flash, survives reflashes).

---

## Web Dashboard

Open the bridge's IP address in a browser:

*   **System banner** with overall state and SEQUENCE ON / OFF controls.
*   **Per-rack cards** showing live current draw (or the offline error reason), per-rack ON/OFF, a link to the strip's own admin page, and an expandable outlet list with click-to-rename.
*   **Settings** for rack names/IPs (up to 8 racks) and hardware pin assignments.
*   **Debug log** link in the footer (see Diagnostics).

## Diagnostics

When something misbehaves, open **`http://<bridge>/log`** (also linked from the dashboard footer). It holds the most recent events with uptime timestamps:

*   Rack online/offline transitions with the exact cause (HTTP error code, timeout, auth rejection, or JSON parse failure with a payload snippet).
*   Which current-sensor endpoint each strip answered on, including the raw payload behind the parsed amps value.
*   Global state transitions (with per-rack offline reasons when entering the error state), queued/failed commands, and OTA events.
*   Boot reset reason (power-on / software / crash / watchdog / brownout).

The same details are exposed per-rack in `/status` (`last_error`, `amps_debug`) and shown in the dashboard. The log lives in RAM and starts fresh each boot; everything is mirrored to Serial at 115200 baud.

## HTTP API

| Endpoint | Method | Description |
| --- | --- | --- |
| `/` | GET | Web dashboard |
| `/status` | GET | Full system state as JSON (racks, outlets, current, errors, pins) |
| `/log` | GET | Diagnostic log (plain text) |
| `/on`, `/off` | GET | Sequence all racks on/off |
| `/rack/{i}/on`, `/rack/{i}/off` | GET | Control a single rack |
| `/rack/{i}/outlet/{o}/rename` | POST | Rename an outlet (`{"name":"..."}`) |
| `/save_config` | POST | Save rack list (JSON array of `{name, ip}`), then restart |
| `/save_pins` | POST | Save GPIO assignments (`{master, led, rack:[...]}`), then restart |

---

## Usage (Simulation)

For development or testing without hardware:

1.  **Install Dependencies**: `pip install flask requests`
2.  **Start Simulator**: `python3 tools/simulator/main.py`
3.  **Access UI**: Open `http://localhost:8003`

---

## Firmware Deployment (ESP32)

1.  Open `firmware/MasterBridge/MasterBridge.ino` in the Arduino IDE.
2.  Install the **ArduinoJson** and **Adafruit_NeoPixel** libraries.
3.  Edit `Config.h` to set your networking mode (Wired/Wi-Fi) and credentials.
    *   Optional: set `USE_STATIC_IP` to `1` and fill in `STATIC_IP` / `STATIC_GATEWAY` / `STATIC_SUBNET` / `STATIC_DNS` to give the bridge a fixed address (default is DHCP).
    *   For wired Ethernet boards, also set `ETH_BOARD` to match your hardware:
        *   `ETH_BOARD_WT32_ETH01_OLIMEX` — WT32-ETH01 / Olimex ESP32-POE (LAN8720 PHY)
        *   `ETH_BOARD_ESP32_P4_ETH` — Waveshare ESP32-P4-ETH (IP101 PHY)
    *   ESP32-P4 boards require **arduino-esp32 core 3.x or newer** (install/update the "esp32" board package via Boards Manager).
    *   Set your Digital Loggers credentials (`DLI_USER` / `DLI_PASS`).
4.  Flash to your hardware over USB.
5.  Subsequent updates can go **over the air**: the bridge appears as a Network Port (`powersequencer`) in the Arduino IDE. Set `OTA_PASSWORD` in `Config.h` to require authentication.

### Updating the embedded dashboard
The web UI is embedded in the firmware as `Dashboard.h`, generated from `web/control-dashboard.html`. After editing the HTML, regenerate it:

```bash
python3 - <<'EOF'
data = open('web/control-dashboard.html', 'rb').read()
body = ', '.join('0x%02x' % b for b in data)
with open('firmware/MasterBridge/Dashboard.h', 'w') as f:
    f.write('#ifndef DASHBOARD_H\n#define DASHBOARD_H\n#include <Arduino.h>\n\n')
    f.write('const uint8_t INDEX_HTML[] PROGMEM = {' + body + '};\n')
    f.write('const size_t INDEX_HTML_SIZE = %d;\n\n#endif\n' % len(data))
EOF
```

---
*Note: This system strictly interfaces with real Digital Loggers hardware via the REST API. Ensure "Allow REST-style API" is enabled on your power strips.*
