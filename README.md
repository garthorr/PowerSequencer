# PowerSequencer Master Bridge

This project is a master control bridge for Digital Loggers Web Power Switch units. It coordinates multiple AV racks, providing synchronized power sequencing and aggregate telemetry monitoring.

The system is designed with a **modular architecture** that can run locally (in Python) for testing/demo purposes and is ready to be ported to **ESP32/Olimex POE** hardware.

## Architecture

*   **SettingsStore**: Manages persistent configuration (Rack Names, IPs).
*   **DigitalLoggersClient**: Handles communication with the DLI REST API.
*   **SequenceEngine**: Executes sequential power-on/off routines with non-blocking delays.
*   **StatusAggregator**: Performs background round-robin polling of all racks.
*   **StateManager**: The source of truth for the system's `ControllerState`.
*   **WebApi**: Provides a RESTful interface for the Dashboard UI.

---

## Local Usage (Demo Mode)

To run the master bridge locally on your computer:

1.  **Install Dependencies**:
    ```bash
    pip install flask requests
    ```
2.  **Start the Bridge**:
    ```bash
    python main.py
    ```
3.  **Access the UI**: Open `http://localhost:8003` in your browser.
    *   By default, it starts in **Demo Mode** with mock data.
    *   To use real racks, edit `settings.json` and set `"demo_mode": false`.

---

## Porting to ESP32 / Olimex POE

The modular structure of the Python implementation maps directly to Arduino/C++ patterns:

| Python Module | ESP32 Implementation |
| :--- | :--- |
| `SettingsStore` | `Preferences.h` (Non-volatile storage) |
| `DigitalLoggersClient` | `HTTPClient.h` |
| `SequenceEngine` | C++ Class using `millis()` for non-blocking delays |
| `StatusAggregator` | Background task or loop logic |
| `StateManager` | Global singleton struct/class |
| `WebApi` | `WebServer.h` (Serving JSON endpoints) |
| `Dashboard UI` | Embedded in `PROGMEM` and served as static content |
| `HardwareInterface` | `digitalRead/Write` for buttons and WS2811 library for LEDs |

### Why Olimex ESP32-POE?
The **Olimex ESP32-POE** is the target hardware for production because:
1.  **Wired Reliability**: Avoids Wi-Fi congestion in dense AV racks.
2.  **Power over Ethernet**: No separate power supply needed for the bridge.
3.  **Stability**: The modular architecture ensures the web server remains responsive even if a rack unit is slow or offline.

---
*Note: This is a refactored version focusing on long-term stability and hardware portability. Privacy is preserved by using placeholders and local settings files.*
