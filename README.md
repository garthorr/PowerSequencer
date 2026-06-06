# PowerSequencer Master Bridge

PowerSequencer is a master control bridge for Digital Loggers Web Power Switch units. It coordinates multiple AV racks, providing synchronized power sequencing and aggregate telemetry monitoring.

This firmware is designed for **ESP32** hardware, with native support for the **WT32-ETH01** (Wired Ethernet) for maximum stability.

## Features

*   **Sequential Power Control**: Automatically staggers power-on and power-off commands across multiple racks with a 1.5s delay to manage inrush current safely.
*   **Web-Based Configuration**: Easily add, remove, or rename racks and update their IP addresses directly through the web dashboard.
*   **Dynamic Rack Management**: Supports up to 8 independent power strips.
*   **Wired Ethernet Support**: Native support for WT32-ETH01 (LAN8720 PHY) to avoid Wi-Fi interference in critical environments.
*   **Persistent Settings**: Configuration is saved to the ESP32's non-volatile memory (Preferences) and survives reboots.
*   **Integrated Dashboard**: A responsive, mobile-friendly control interface served directly from the hardware.
*   **Hardware Feedback**:
    *   **Physical Button**: Toggle the entire system via a button on GPIO 12.
    *   **LED Patterns**: Visual status on GPIO 14 (Solid = ON, Pulsing = Sequencing, Fast Blink = Error/Mixed).

---

## Hardware Setup (WT32-ETH01)

The WT32-ETH01 is the recommended hardware for "tank-like" reliability.

### Pinout
*   **Button**: Connect a momentary push button between **IO12** and **GND**.
    *   *Note: IO12 is a strapping pin. Ensure the button is not held down during initial power-up/reboot.*
*   **LED**: Connect an LED (with 220Ω resistor) to **IO14**.

---

## Getting Started

### 1. Firmware Configuration
1.  Open `webPowerSwitchAllOutlets` in the Arduino IDE.
2.  Install the **ArduinoJson** library via the Library Manager.
3.  In the **USER CONFIGURATION** section:
    *   Set `#define USE_ETHERNET 1` if using a WT32-ETH01.
    *   Set your Wi-Fi credentials if using a standard ESP32 in Wi-Fi mode.
    *   Ensure `powerSwitchUser` and `powerSwitchPass` match your Digital Loggers admin credentials.
4.  Flash the firmware to your device.

### 2. Web Configuration (For Non-Tech Personnel)
1.  Find the bridge IP address in the Serial Monitor (or your router's client list).
2.  Navigate to that IP in any web browser.
3.  Click the **⚙ Settings** button in the header.
4.  **Manage Racks**:
    *   **Add Rack**: Click "+ Add Rack" to add a new unit.
    *   **Edit**: Enter the Name and IP address for each rack.
    *   **Remove**: Click the "✕" button next to a rack to remove it.
5.  Click **Save & Reboot**. The bridge will restart and begin managing the new configuration.

---

## Troubleshooting
*   **Bridge won't boot**: Ensure no button is connected to IO12 that might be pulling the pin LOW during startup.
*   **Racks show "CONN ERROR"**: Verify the IP address in Settings and ensure the Digital Loggers unit has "Allow REST-style API" enabled in its External APIs settings.
*   **Dashboard is slow**: The bridge polls one rack per second to ensure network stability. For 4 racks, a full refresh takes 4 seconds.

---
*Note: This is a public repository. Never commit your private Wi-Fi credentials or internal rack IP addresses.*
