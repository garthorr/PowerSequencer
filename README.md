# PowerSequencer Master Bridge

PowerSequencer is a master control bridge for Digital Loggers Web Power Switch units. It coordinates multiple AV racks, providing synchronized power sequencing and aggregate telemetry monitoring.

This firmware is designed for **ESP32** hardware, with native support for the **WT32-ETH01** (Wired Ethernet) for maximum stability.

## Features

*   **Sequential Power Control**: Automatically staggers power-on and power-off commands across multiple racks with a 1.5s delay to manage inrush current.
*   **Web-Based Configuration**: Non-technical personnel can configure Rack Names and IP addresses directly through the web dashboard. Settings are saved to persistent memory.
*   **Wired Ethernet Support**: Native support for WT32-ETH01 (LAN8720 PHY) to avoid Wi-Fi interference in critical environments.
*   **Integrated Dashboard**: A responsive, mobile-friendly control interface served directly from the hardware.
*   **Hardware Feedback**:
    *   **Physical Button**: Toggle the entire system via a button on GPIO 12.
    *   **LED Patterns**: Visual status on GPIO 14 (Solid = ON, Pulsing = Sequencing, Fast Blink = Error).

---

## Hardware Setup (WT32-ETH01)

The WT32-ETH01 is the recommended hardware for "tank-like" reliability.

### Pinout
*   **Button**: Connect a momentary push button between **IO12** and **GND**.
*   **LED**: Connect an LED (with 220Ω resistor) to **IO14**.

---

## Getting Started

### 1. Firmware Configuration
1.  Open `webPowerSwitchAllOutlets` in the Arduino IDE.
2.  Install the **ArduinoJson** library.
3.  In the **USER CONFIGURATION** section:
    *   Set `#define USE_ETHERNET 1` if using a WT32-ETH01.
    *   Set your Wi-Fi credentials if using a standard ESP32 in Wi-Fi mode.
4.  Flash the firmware to your device.

### 2. Initial Setup
1.  Find the bridge IP address in the Serial Monitor (or your router's client list).
2.  Navigate to that IP in any web browser.
3.  Click the **⚙ Settings** button in the header.
4.  Enter the names and IP addresses for your Digital Loggers power strips.
5.  Click **Save & Reboot**.

---

## API Reference
*   `GET /`: Serves the control dashboard.
*   `GET /status`: Returns JSON global state and per-rack telemetry.
*   `GET /on`: Triggers the sequential power-on routine.
*   `GET /off`: Triggers the sequential power-off routine.
*   `POST /save_config`: Saves rack configuration to persistent storage.

---
*Note: This is a public repository. Never commit your private Wi-Fi credentials or internal rack IP addresses.*
