Ultra Low Cost LoRa Gateway with ESP8266 modules
Based on professor Congduc Pham's sx1272 project.
It uses low cost ESP8266 modules for sensor nodes (even without Wifi support) and for the LoRa Gateway, which receives messages from the sensors and publish them on the Thingspeak services
The SX1272 library was changed to include "yield()" statements in the receive process loop. This statement free the processor to do other tasks, avoiding crashes.
The conection to the HopeRF RFM95W LoRa Module is made using the standard SPI pins on ESP8266 (12,13,14 and 15), but the CS pin can be changed in sx1272.h file.
The support for ESP8266 modules on the Arduino IDE is documented in https://github.com/esp8266/Arduino
