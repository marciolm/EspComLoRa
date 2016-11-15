/*  
 *  LoRa 868 / 915MHz SX1272 LoRa module
 *  
 *  Copyright (C) Libelium Comunicaciones Distribuidas S.L. 
 *  http://www.libelium.com 
 *  
 *  This program is free software: you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation, either version 3 of the License, or 
 *  (at your option) any later version. 
 *  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License 
 *  along with this program.  If not, see http://www.gnu.org/licenses/. 
 *  
 *  Version:           1.1
 *  Design:            David Gascón 
 *  Implementation:    Covadonga Albiñana & Victor Boria
 */

// Include the SX1272 and SPI library:
#include "SX1272.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#define SX1272_debug_mode 2
#define RADIO_RFM92_95
#define BAND900
String apiKey = "thingspeak_api_key";
const char* ssid = "ssid";
const char* password = "password";
const char* server = "api.thingspeak.com";
int e;
char my_packet[100];
int led = 2 ;
boolean led_state = LOW;
WiFiClient client;
String my_packet_str;
void setup()
{
  int nCnt = 0;
   // Open serial communications and wait for port to open:
  Serial.begin(9600);
     
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    nCnt ++;
    if(nCnt > 20) break;

  }
  Serial.println("");
  Serial.println("WiFi connected");

  pinMode(led, OUTPUT);
 
  // Print a start message
  Serial.println(F("SX1272 module and Arduino: receive packets without ACK"));

  // Power ON the module
  e = sx1272.ON();
  Serial.print(F("Setting power ON: state "));
  Serial.println(e, DEC);
  
  // Set transmission mode and print the result
  e = sx1272.setMode(1);
  Serial.print(F("Setting Mode: state "));
  Serial.println(e, DEC);
  
  // Set header
  e = sx1272.setHeaderON();
  Serial.print(F("Setting Header ON: state "));
  Serial.println(e, DEC);
  
  // Select frequency channel
  e = sx1272.setChannel(CH_05_900);
  Serial.print(F("Setting Channel: state "));
  Serial.println(e, DEC);
  
  
  // Select output power (Max, High or Low)
  e = sx1272.setPower('x');
  Serial.print(F("Setting Power: state "));
  Serial.println(e, DEC);
  
  // Set the node address and print the result
  e = sx1272.setNodeAddress(1);
  Serial.print(F("Setting node address: state "));
  Serial.println(e, DEC);
  
  // Print a success message
  Serial.println(F("SX1272 successfully configured"));
  Serial.println();
}

void loop(void)
{

  // Receive message
  e = sx1272.receivePacketTimeout(10000);
  if ( e == 0 )
  {
    Serial.print(F("Receive packet, state "));
    Serial.println(e, DEC);

    for (unsigned int i = 0; i < sx1272.packet_received.length; i++)
    {
      my_packet[i] = (char)sx1272.packet_received.data[i];
    }
    my_packet_str = String(my_packet);
    int Index1 = my_packet_str.indexOf('#');
    int Index2 = my_packet_str.indexOf('#', Index1+1);
    int Index3 = my_packet_str.indexOf('#', Index2+1);

    String secondValue = my_packet_str.substring(Index1+1, Index2);
    String thirdValue = my_packet_str.substring(Index2+1, Index3);

    Serial.println(secondValue);
    Serial.println(thirdValue);  
    Serial.print(F("Message: "));
    Serial.println(my_packet);
    led_state = !led_state ;
    digitalWrite(led, led_state);  // toggle the led state
    
    sx1272.getRSSIpacket();
    sx1272.getSNR();

    if (client.connect(server,80)) {  //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
    if (secondValue == "2") {
           postStr +="&field1=";
           postStr += String(sx1272._RSSIpacket);
           postStr +="&field2=";
           postStr += String(sx1272._SNR);
           postStr +="&field3=";
           postStr += thirdValue;
           postStr += "\r\n\r\n";
    }
    if (secondValue == "4") {
           postStr +="&field4=";
           postStr += String(sx1272._RSSIpacket);
           postStr +="&field5=";
           postStr += String(sx1272._SNR);
           postStr +="&field6=";
           postStr += thirdValue;
           postStr += "\r\n\r\n";
    }
     client.print("POST /update HTTP/1.1\n"); 
     client.print("Host: api.thingspeak.com\n"); 
     client.print("Connection: close\n"); 
     client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n"); 
     client.print("Content-Length: "); 
     client.print(postStr.length()); 
     client.print("\n\n"); 
     client.print(postStr);
     Serial.println("Message sent to Thingspeak");
     client.stop();
    }     
    
    Serial.print("Console: RSSI is ");
    Serial.print(sx1272._RSSIpacket,DEC); 
    Serial.print(" dBm");
    Serial.println(" ");    
    Serial.print("Console: SNR is ");
    Serial.print(sx1272._SNR,DEC); 
    Serial.println(" dBm");
   }
  else {
    Serial.print(F("Receive packet, state "));
    Serial.println(e, DEC);
  }
}
