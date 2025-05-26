# PowerSequencer
Building an IP-controlled power sequencer.

Overview

This system enables centralized and remote control of four AV equipment zones using Web Power Switch Pro units. Each station is controlled via a WT32-ETH01 ESP32 module using wired Ethernet as the primary connection, with Wi-Fi as a backup. A base station with an Arduino Mega acts as the system controller, providing physical controls and status indicators.

⸻

System Components

1. Base Station (Front of House)

	•	Hardware:
	•	Arduino Mega  
	•	Wi-Fi module (ESP8266 or ESP32) for HTTP client communication  
	•	1x momentary push button (master toggle)  
	•	4x power status LEDs (Amp Rack, Stage 1, Stage 2, Base Station)  
	•	3x connectivity LEDs (Amp Rack, Stage 1, Stage 2)  
	•	Functions:  
	•	Press button to toggle power for all stations  
	•	Poll remote stations for power and connectivity status  
	•	Drive LED indicators based on remote station responses  
	•	Optional: host a simple web interface for remote control  

⸻

2. Remote Stations (x4)

	•	Locations: Amp Rack, Stage Rack 1, Stage Rack 2, Base Rack  
	•	Hardware:  
	•	WT32-ETH01 ESP32 module  
	•	Web Power Switch Pro (controlled via HTTP API)  
	•	Manual override push button  
	•	Local relay (optional for control fallback)  
	•	Functions:  
	•	Serve HTTP endpoints:  
	•	GET /on – powers ON the rack  
	•	GET /off – powers OFF the rack  
	•	GET /toggle – toggles current state  
	•	GET /status – returns current power state and connection mode  
	•	Poll local Web Power Switch Pro to confirm status  
	•	Respond to commands from base station  
	•	Automatically prefer Ethernet; fallback to Wi-Fi if Ethernet unavailable  
	•	Manual override button powers ON/OFF locally regardless of network state  

⸻

Communication Flow

Base Station → WT32 API Calls
REST documentatin can be found [here](https://www.digital-loggers.com/restapi.pdf)  

	•	http://<IP>/on → turn on station
	•	http://<IP>/off → turn off station
	•	http://<IP>/status → returns JSON:

{
  "power": "on",
  "connection": "ethernet", 
  "reachable": true
}

Failover Logic
	•	Try primary (Ethernet) IP  
	•	If unreachable after 3 attempts, try fallback Wi-Fi IP  
	•	Mark as offline if both fail  

⸻

Web Control Panel (Optional)
	•	Served from Arduino Mega or a separate server  
	•	Displays:  
	•	Toggle buttons per station  
	•	Status indicators (power and connectivity)  
	•	Sends same HTTP commands as hardware  

⸻

Status LED Logic (on Base Station)  
	•	Power LEDs (4):  
	•	ON: confirmed “on” from /status  
	•	OFF: confirmed “off”  
	•	Blinking/Red: no response  
	•	Connectivity LEDs (3):  
	•	Green: /status responded successfully  
	•	Red: no connection via either IP  

⸻

Developer Tasks

For Each WT32-ETH01 Station  
	•	Implement Ethernet-first networking with Wi-Fi fallback  
	•	Serve RESTful API for /on, /off, /toggle, /status  
	•	Integrate Web Power Switch control (HTTP GET/POST)  
	•	Poll switch status every 5–10 sec  
	•	Monitor manual override button via GPIO  

For Arduino Mega Base Station  
	•	Periodically ping each WT32 station  
	•	Send HTTP commands on button press  
	•	Drive LED indicators based on power and connectivity  
	•	Optional: implement simple web control interface  
