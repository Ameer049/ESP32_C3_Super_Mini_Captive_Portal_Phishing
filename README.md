This project is a security-awareness demonstration that shows how a malicious actor could mimic a public Wi-Fi login page using an ESP32 device.
It creates a fake “free WiFi” access point with a captive portal, displays a realistic-looking login flow, and records submitted credentials to an SD card.

The purpose of this project is to educate users about:

* The risks of connecting to unknown Wi-Fi networks

* How captive-portal phishing works

* Why you should never trust login prompts on untrusted networks

This project must only be used in authorized labs, training sessions, and penetration-testing simulations.

🔧 Features
* Creates an open Wi-Fi network using the ESP32

* Forces all connected clients to a captive-portal landing page

* Hosts multiple HTML pages simulating a login flow

* Logs submitted form data (email + password) to log.txt on the SD card

* Uses DNS spoofing to redirect all traffic to the ESP32

* Stores logs on SD using SPI

* Displays data on the serial monitor for debugging

⚙️ Hardware Requirements
* ESP32-C3 Super Mini (or compatible ESP32 board)
* MicroSD card + SD card adapter
* USB cable
* (Optional) Powerbank for mobile demonstrations

📁 File Logging
All submitted form fields are written to:
/log.txt

📦 Installation
1. Install Required Libraries
* WiFi.h

* WebServer.h

* DNSServer.h

* SD.h

* SPI.h

2. Configure Correct SD Pins
* ESP32 to SD card module
* Ground -> Ground
* 3.3 V  -> 3.3 V
* Pin 4  -> MISO
* Pin 5  -> CLK
* Pin 6  -> MOSI
* Pin 7  -> CS
