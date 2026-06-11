#ifndef CONFIG_H
#define CONFIG_H

// --- Networking Mode ---
#define USE_ETHERNET 1  // 1 for wired Ethernet boards, 0 for Wi-Fi

// --- Ethernet Board Selection (only used when USE_ETHERNET == 1) ---
// ETH_BOARD_WT32_ETH01_OLIMEX : WT32-ETH01 / Olimex ESP32-POE (classic ESP32, LAN8720 PHY)
// ETH_BOARD_ESP32_P4_ETH      : Waveshare ESP32-P4-ETH (ESP32-P4, IP101 PHY)
// Requires arduino-esp32 core 3.x or newer for ESP32-P4 support.
#define ETH_BOARD_WT32_ETH01_OLIMEX 1
#define ETH_BOARD_ESP32_P4_ETH      2
#define ETH_BOARD ETH_BOARD_ESP32_P4_ETH

// --- Wi-Fi Settings ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// --- DLI Credentials ---
const char* DLI_USER = "admin";
const char* DLI_PASS = "1234";

// --- OTA Updates (Arduino IDE / arduino-cli "Network Port") ---
#define OTA_HOSTNAME "powersequencer"
// Leave empty to allow unauthenticated OTA on the LAN.
#define OTA_PASSWORD ""

// --- Hardware ---
#define MASTER_BUTTON_PIN 12
#define LED_PIN           14
#define NUM_LEDS          8  // Support up to 8 racks

// Individual Rack Buttons (GPIOs 35, 36, 39 are Input Only)
const uint8_t RACK_BUTTON_PINS[] = {35, 36, 39, 15, 4, 32, 33};

// --- Timing ---
const uint32_t SEQUENCE_DELAY_MS = 1500;
const uint32_t POLL_INTERVAL_MS   = 1000;

#endif
