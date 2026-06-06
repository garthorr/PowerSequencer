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
*   **Toggle Button**: Momentary button between **IO12** and **GND**.
*   **Status LED**: LED (with 220Ω resistor) on **IO14**.
    *   **Solid**: System is fully ON.
    *   **Off**: System is fully OFF.
    *   **Pulsing**: Sequencing in progress.
    *   **Fast Blink**: Error or Mixed state detected.

---

## Usage (Local Python)

For testing or using a PC as the master bridge:

1.  **Install Dependencies**: `pip install flask requests`
2.  **Start Bridge**: `python3 main.py`
3.  **Access UI**: Open `http://localhost:8003`
4.  **Configure**: Go to **⚙ Settings** and enter your real rack IP addresses.

---

## Firmware Deployment (ESP32)

1.  Open `firmware/MasterBridge/MasterBridge.ino` in the Arduino IDE.
2.  Install the **ArduinoJson** library.
3.  Edit `Config.h` to set your networking mode (Wired/Wi-Fi) and credentials.
4.  Flash to your hardware.

---
*Note: This system strictly interfaces with real Digital Loggers hardware via the REST API. Ensure "Allow REST-style API" is enabled on your power strips.*
