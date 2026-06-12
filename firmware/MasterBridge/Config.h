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

// --- Static IP (applies to both Ethernet and Wi-Fi) ---
// Set USE_STATIC_IP to 1 to use the addresses below; 0 uses DHCP.
#define USE_STATIC_IP 0
#define STATIC_IP      192, 168, 0, 50
#define STATIC_GATEWAY 192, 168, 0, 1
#define STATIC_SUBNET  255, 255, 255, 0
#define STATIC_DNS     192, 168, 0, 1

// --- DLI Credentials ---
const char* DLI_USER = "admin";
const char* DLI_PASS = "1234";

// --- OTA Updates (Arduino IDE / arduino-cli "Network Port") ---
#define OTA_HOSTNAME "powersequencer"
// Leave empty to allow unauthenticated OTA on the LAN.
#define OTA_PASSWORD ""

// --- Hardware ---
#define NUM_LEDS 8  // Support up to 8 racks

// LED color order + data rate. Most NeoPixels (including Adafruit's RGB
// thru-hole 5mm/8mm pixels) are WS2812-compatible at 800 KHz; only the
// color order differs from the GRB SMD norm. If colors are stuck/garbled
// rather than just swapped, try NEO_KHZ400 instead.
#define LED_TYPE (NEO_RGB + NEO_KHZ800)

#if USE_ETHERNET && (ETH_BOARD == ETH_BOARD_ESP32_P4_ETH)
// Waveshare ESP32-P4-ETH header. The classic-ESP32 pins (12, 35, 36, 39)
// don't exist on this board. GPIO24/25 (USB D-/D+) and GPIO7/8 (I2C) are
// left free; remaining header GPIOs (23, 26, 27, 32, 33, 46-48, 54...)
// are available for expansion.
#define MASTER_BUTTON_PIN 15
#define LED_PIN           14
const uint8_t RACK_BUTTON_PINS[] = {16, 17, 18, 19, 20, 21, 22};
#else
// WT32-ETH01 / Olimex ESP32-POE / generic classic ESP32
#define MASTER_BUTTON_PIN 12
#define LED_PIN           14
// Individual Rack Buttons (GPIOs 35, 36, 39 are Input Only)
const uint8_t RACK_BUTTON_PINS[] = {35, 36, 39, 15, 4, 32, 33};
#endif

// --- Timing ---
const uint32_t SEQUENCE_DELAY_MS = 1500;
const uint32_t POLL_INTERVAL_MS   = 1000;

#endif
