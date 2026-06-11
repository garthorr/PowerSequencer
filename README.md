# PowerSequencer Master Bridge

PowerSequencer is a professional-grade master control bridge for Digital Loggers Web Power Switch units. It coordinates multiple AV racks, providing synchronized power sequencing and detailed telemetry monitoring from a single interface.

This project is built on a **modular architecture** designed for high stability and industrial deployment on **ESP32** or **Olimex ESP32-POE** hardware.

## Features

*   **Sequential Power Control**: Automatically staggers power commands across racks with a 1.5s delay to manage inrush current safely.
*   **Deep Telemetry**: Real-time status for the whole system, individual racks, and **individual outlets** (including names and states).
*   **Wired Reliability**: Native support for LAN8720 PHY (WT32-ETH01 / Olimex) for mission-critical wired networking.
*   **Web-Based Configuration**: Manage rack names and IP addresses directly through the dashboard. Settings persist in non-volatile memory.
*   **Hardware Integration**: Physical toggle button support and real-time status LED patterns.
*   **Production Ready**: Non-blocking background polling ensures the bridge remains responsive even if network units are offline.

---

## Hardware Setup (Recommended: WT32-ETH01)

The bridge is designed to be a reliable hardware appliance.

### Pinout
*   **Master Button**: Momentary button between **IO12** and **GND**.
*   **Rack Buttons**: Individual buttons for racks 1-7 on **GPIOs 35, 36, 39, 15, 4, 32, 33**.
*   **Status LEDs (WS2811)**: Data pin on **IO14**.
    *   **Green**: Rack is ON.
    *   **Dim Red**: Rack is OFF.
    *   **Orange**: Mixed outlet states.
    *   **Flashing Blue**: Sequencing in progress.
    *   **Flashing Red**: Rack is Offline.

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
4.  Flash to your hardware.

---
*Note: This system strictly interfaces with real Digital Loggers hardware via the REST API. Ensure "Allow REST-style API" is enabled on your power strips.*
