#ifndef CONFIG_H
#define CONFIG_H

// --- Networking Mode ---
#define USE_ETHERNET 1  // 1 for WT32-ETH01 / Olimex, 0 for Wi-Fi

// --- Wi-Fi Settings ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// --- DLI Credentials ---
const char* DLI_USER = "admin";
const char* DLI_PASS = "1234";

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
