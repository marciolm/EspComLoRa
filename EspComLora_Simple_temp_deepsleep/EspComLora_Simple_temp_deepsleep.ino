/*
 *  temperature sensor on analog 8 to test the LoRa gateway
 *
 *  Copyright (C) 2016 Congduc Pham, University of Pau, France
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************
 */
#include <SPI.h> 
// Include the SX1272
#include "SX1272.h"
#include <EEPROM.h>

// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// please uncomment only 1 choice
//
// uncomment if your radio is an HopeRF RFM92W or RFM95W
#define RADIO_RFM92_95
// uncomment if your radio is a Modtronix inAirB (the one with +20dBm features), if inAir9, leave commented
//#define RADIO_INAIR9B
// uncomment if you only know that it has 20dBm feature
//#define RADIO_20DBM
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// please uncomment only 1 choice
//#define BAND868
#define BAND900
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// COMMENT OR UNCOMMENT TO CHANGE FEATURES. 
// ONLY IF YOU KNOW WHAT YOU ARE DOING!!! OTHERWISE LEAVE AS IT IS
#define WITH_EEPROM
#define WITH_APPKEY
#define FLOAT_TEMP
#define NEW_DATA_FIELD
#define LOW_POWER
//#define LOW_POWER_HIBERNATE
//#define WITH_ACK
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE LORA MODE, NODE ADDRESS 
#define LORAMODE  1
#define node_addr 5
//////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE THINGSPEAK FIELD BETWEEN 1 AND 4
#define field_index 2
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE READ PIN AND THE POWER PIN FOR THE TEMP. SENSOR
#define TEMP_PIN_READ  A0
// use digital 8 to power the temperature sensor if needed
#define TEMP_PIN_POWER 2
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE TIME IN MINUTES BETWEEN 2 READING & TRANSMISSION
unsigned int idlePeriodInMin = 10;
///////////////////////////////////////////////////////////////////

#ifdef WITH_APPKEY
///////////////////////////////////////////////////////////////////
// CHANGE HERE THE APPKEY, BUT IF GW CHECKS FOR APPKEY, MUST BE
// IN THE APPKEY LIST MAINTAINED BY GW.
uint8_t my_appKey[4]={5, 6, 7, 8};
///////////////////////////////////////////////////////////////////
#endif

#define PRINTLN                   Serial.println("")
#define PRINT_CSTSTR(fmt,param)   Serial.print(F(param))
#define PRINT_STR(fmt,param)      Serial.print(param)
#define PRINT_VALUE(fmt,param)    Serial.print(param)
#define FLUSHOUTPUT               Serial.flush();




#define DEFAULT_DEST_ADDR 1

#ifdef WITH_ACK
#define NB_RETRIES 2
#endif

#define TEMP_SCALE  3300.0


double temp;
unsigned long lastTransmissionTime=0;
unsigned long delayBeforeTransmit=0;
char float_str[20];
uint8_t message[100];
int loraMode=LORAMODE;


struct sx1272config {

  uint8_t flag1;
  uint8_t flag2;
  uint8_t seq;
  // can add other fields such as LoRa mode,...
};

sx1272config my_sx1272config;


void setup()
{
  int e;

  // for the temperature sensor
  pinMode(TEMP_PIN_READ, INPUT);
  // and to power the temperature sensor
  pinMode(TEMP_PIN_POWER,OUTPUT);

#ifdef LOW_POWER
  digitalWrite(TEMP_PIN_POWER,HIGH);
#endif

delay(3000);
// Open serial communications and wait for port to open:
Serial.begin(9600);

  // Print a start message
  PRINT_CSTSTR("%s","Simple LoRa temperature sensor\n");


  // Power ON the module
  sx1272.ON();

  EEPROM.begin(512);
  EEPROM.get(0, my_sx1272config);

  // found a valid config?
  if (my_sx1272config.flag1==0x12 && my_sx1272config.flag2==0x34) {
    PRINT_CSTSTR("%s","Get back previous sx1272 config\n");

    // set sequence number for SX1272 library
    sx1272._packetNumber=my_sx1272config.seq;
    PRINT_CSTSTR("%s","Using packet sequence number of ");
    PRINT_VALUE("%d", sx1272._packetNumber);
    PRINTLN;
  }
  else {
    // otherwise, write config and start over
    my_sx1272config.flag1=0x12;
    my_sx1272config.flag2=0x34;
    my_sx1272config.seq=sx1272._packetNumber;
    PRINT_CSTSTR("%s","New packet sequence");
    PRINTLN;
  }

  
  // Set transmission mode and print the result
  e = sx1272.setMode(loraMode);
  PRINT_CSTSTR("%s","Setting Mode: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // enable carrier sense
  sx1272._enableCarrierSense=true;
#ifdef LOW_POWER
  // TODO: with low power, when setting the radio module in sleep mode
  // there seem to be some issue with RSSI reading
  sx1272._RSSIonSend=false;
#endif   
    
#ifdef BAND868
  // Select frequency channel
  e = sx1272.setChannel(CH_10_868);
#else // assuming #defined BAND900
  // Select frequency channel
  e = sx1272.setChannel(CH_05_900);
#endif
  PRINT_CSTSTR("%s","Setting Channel: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // Select output power (Max, High or Low)
#if defined RADIO_RFM92_95 || defined RADIO_INAIR9B
  e = sx1272.setPower('x');
#else
  e = sx1272.setPower('M');
#endif  

  PRINT_CSTSTR("%s","Setting Power: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // Set the node address and print the result
  e = sx1272.setNodeAddress(node_addr);
  PRINT_CSTSTR("%s","Setting node addr: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // Print a success message
  PRINT_CSTSTR("%s","SX1272 successfully configured\n");

  delay(500);
}

char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}


void loop(void)
{
  long startSend;
  long endSend;
  uint8_t app_key_offset=0;
  int e;

#ifndef LOW_POWER
  // 600000+random(15,60)*1000
  if (millis()-lastTransmissionTime > delayBeforeTransmit) {
#endif
#ifdef LOW_POWER
      digitalWrite(TEMP_PIN_POWER,HIGH);
      // security?
      delay(200);    
      int value = analogRead(TEMP_PIN_READ);
      digitalWrite(TEMP_PIN_POWER,LOW);
#else
      int value = analogRead(TEMP_PIN_READ);
#endif
      
      // change here how the temperature should be computed depending on your sensor type
      //  
      temp = value*TEMP_SCALE/1024.0;
    
      PRINT_CSTSTR("%s","Reading ");
      PRINT_VALUE("%d", value);
      PRINTLN;
      
      //temp = temp - 0.5;
      temp = temp / 10.0;

      PRINT_CSTSTR("%s","Temp is ");
      PRINT_VALUE("%f", temp);
      PRINTLN;
      
#ifdef WITH_APPKEY
      app_key_offset = sizeof(my_appKey);
      // set the app key in the payload
      memcpy(message,my_appKey,app_key_offset);
#endif

      uint8_t r_size;

      // then use app_key_offset to skip the app key
    
#ifdef FLOAT_TEMP
      ftoa(float_str,temp,2);

#ifdef NEW_DATA_FIELD
      // this is for testing, uncomment if you just want to test, without a real temp sensor plugged
      //strcpy(float_str, "21.55567");
      r_size=sprintf((char*)message+app_key_offset,"\\!#%d#%s",field_index,float_str);
#else
      // this is for testing, uncomment if you just want to test, without a real temp sensor plugged
      //strcpy(float_str, "21.55567");
      r_size=sprintf((char*)message+app_key_offset,"\\!#%d#%s",field_index,float_str);
#endif
      
#else
      
#ifdef NEW_DATA_FIELD      
      r_size=sprintf((char*)message+app_key_offset, "\\!#%d#%d", field_index, (int)temp);   
#else
      r_size=sprintf((char*)message+app_key_offset, "\\!#%d#%d", field_index, (int)temp);
#endif         
#endif


      PRINT_CSTSTR("%s","Sending ");
      PRINT_STR("%s",(char*)(message+app_key_offset));
      PRINTLN;
      
      PRINT_CSTSTR("%s","Real payload size is ");
      PRINT_VALUE("%d", r_size);
      PRINTLN;
      
      int pl=r_size+app_key_offset;
      
      sx1272.CarrierSense();
      
      startSend=millis();

#ifdef WITH_APPKEY
      // indicate that we have an appkey
      sx1272.setPacketType(PKT_TYPE_DATA | PKT_FLAG_DATA_WAPPKEY);
#else
      // just a simple data packet
      sx1272.setPacketType(PKT_TYPE_DATA);
#endif
      
      // Send message to the gateway and print the result
      // with the app key if this feature is enabled
#ifdef WITH_ACK
      int n_retry=NB_RETRIES;
      
      do {
        e = sx1272.sendPacketTimeoutACK(DEFAULT_DEST_ADDR, message, pl);

        if (e==3)
          PRINT_CSTSTR("%s","No ACK");
        
        n_retry--;
        
        if (n_retry)
          PRINT_CSTSTR("%s","Retry");
        else
          PRINT_CSTSTR("%s","Abort"); 
          
      } while (e && n_retry);          
#else      
      e = sx1272.sendPacketTimeout(DEFAULT_DEST_ADDR, message, pl);
#endif  
      endSend=millis();
    

      // save packet number for next packet in case of reboot
      my_sx1272config.seq=sx1272._packetNumber;
      EEPROM.put(0, my_sx1272config);
      EEPROM.commit();

      PRINT_CSTSTR("%s","LoRa pkt size ");
      PRINT_VALUE("%d", pl);
      PRINTLN;
      
      PRINT_CSTSTR("%s","LoRa pkt seq ");
      PRINT_VALUE("%d", sx1272.packet_sent.packnum);
      PRINTLN;
    
      PRINT_CSTSTR("%s","LoRa Sent in ");
      PRINT_VALUE("%ld", endSend-startSend);
      PRINTLN;
          
      PRINT_CSTSTR("%s","LoRa Sent w/CAD in ");
      PRINT_VALUE("%ld", endSend-sx1272._startDoCad);
      PRINTLN;

      PRINT_CSTSTR("%s","Packet sent, state ");
      PRINT_VALUE("%d", e);
      PRINTLN;
        
#ifdef LOW_POWER
      PRINT_CSTSTR("%s","Switch to power saving mode\n");

      e = sx1272.setSleepMode();

      if (!e)
        PRINT_CSTSTR("%s","Successfully switch LoRa module in sleep mode\n");
      else  
        PRINT_CSTSTR("%s","Could not switch LoRa module in sleep mode\n");
        
      FLUSHOUTPUT
      delay(50);

      // use a random part also to avoid collision
      PRINT_CSTSTR("%s","Entering ESP Deep Sleep");
      PRINTLN;
      ESP.deepSleep(600000000);
      
  #else
      // use a random part also to avoid collision
      PRINT_VALUE("%ld", lastTransmissionTime);
      PRINTLN;
      PRINT_CSTSTR("%s","Will send next value in\n");
      lastTransmissionTime=millis();
      delayBeforeTransmit=idlePeriodInMin*60*1000+random(15,60)*1000;
      PRINT_VALUE("%ld", delayBeforeTransmit);
      PRINTLN;

#endif

  delay(50);
}

