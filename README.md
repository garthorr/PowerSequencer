# PowerSequencer

PowerSequencer is a master control bridge for Digital Loggers Web Power Switch units. It allows a single operator to manage multiple AV racks, providing synchronized power sequencing and aggregate telemetry monitoring.

The system consists of an ESP8266 or ESP32 bridge that coordinates communication between the user and multiple power strips.

## Features

*   **Sequential Power Control**: Automatically staggers power-on and power-off commands across multiple racks with a configurable delay (default 1.5s) to prevent circuit breaker trips from high inrush current.
*   **Integrated Dashboard**: A responsive, mobile-friendly web dashboard is served directly from the bridge hardware, providing real-time status and telemetry.
*   **Aggregate Telemetry**: Monitors current draw from all connected strips and displays total system load.
*   **Hardware Feedback**: Physical status LED provides instant visual feedback:
    *   **Solid**: System is fully ON.
    *   **Off**: System is fully OFF.
    *   **Pulsing**: Sequencing in progress.
    *   **Fast Blink**: Error/Communication failure.
*   **Physical Override**: Supports a physical push-button to toggle the entire system without needing the web UI.
*   **Privacy First**: No cloud dependency. All communication stays within your local network.

## Hardware Selection & Stability

While an Arduino Mega is a classic choice, this project **requires an ESP32 or ESP8266**.

The master bridge performs heavy JSON parsing and serves a modern web dashboard. The ESP32's 520KB of RAM allows for safe, buffered communication with multiple power strips simultaneously. In contrast, the Arduino Mega (with only 8KB of RAM) would likely crash or experience memory corruption when handling the large JSON telemetry payloads from multiple Digital Loggers units.

For **Industrial-Grade Stability**:
If you require maximum reliability (avoiding Wi-Fi interference), we recommend using an ESP32 with **wired Ethernet**, such as the **WT32-ETH01**. The firmware is compatible with both Wi-Fi and Ethernet ESP32 variants.

## Hardware Requirements

*   **Microcontroller**: ESP32 (Recommended) or ESP8266.
*   **Power Strips**: Digital Loggers Web Power Switch Pro (or any DLI model supporting the REST API).
*   **Components**:
    *   1x Push button (connected to GPIO 12 / D6).
    *   1x LED with resistor (connected to GPIO 14 / D5).

## Getting Started

### 1. Firmware Configuration

1.  Open `webPowerSwitchAllOutlets` in the Arduino IDE.
2.  Install the **ArduinoJson** library via the Library Manager.
3.  Update the **USER CONFIGURATION** section at the top of the file:
    *   Set your Wi-Fi SSID and Password.
    *   Enter the IP addresses and names for your power strips in the `powerStrips` array.
    *   Configure `SEQUENCE_DELAY_MS` if your equipment requires more time between racks.

### 2. Deployment

1.  Flash the firmware to your ESP board.
2.  Open the Serial Monitor (115200 baud) to find the IP address assigned to the bridge.
3.  Navigate to that IP address in any web browser to access the control dashboard.

## API Reference

The bridge exposes the following endpoints:

*   `GET /`: Serves the integrated control dashboard.
*   `GET /status`: Returns a JSON object containing global state and per-strip telemetry.
*   `GET /on`: Triggers the sequential power-on routine.
*   `GET /off`: Triggers the sequential power-off routine.

## Development Notes

*   **Stability**: Communication uses optimized batch commands (`?a=ON/OFF`) and caches successful telemetry endpoints to ensure the UI remains snappy even with many strips.
*   **Memory**: Uses `DynamicJsonDocument` to handle varying numbers of connected devices safely.
*   **CORS**: The API supports Cross-Origin Resource Sharing, allowing it to be integrated into larger monitoring systems if desired.

---
*Note: This is a public repository. Ensure you never commit your actual Wi-Fi credentials or private IP addresses.*
