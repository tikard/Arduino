#include <Arduino.h>

// nrf24_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_NRF24 driver to control a NRF24 radio.
// It is designed to work with the other example nrf24_reliable_datagram_server
// Tested on Uno with Sparkfun WRL-00691 NRF24L01 module
// Tested on Teensy with Sparkfun WRL-00691 NRF24L01 module
// Tested on Anarduino Mini (http://www.anarduino.com/mini/) with RFM73 module
// Tested on Arduino Mega with Sparkfun WRL-00691 NRF25L01 module
//
// Got Working with 5V adaptor boards BUT NOTE:
//               MISO and MIS1 are reversed on these durn things
//

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

#if !defined(nullptr)
#define nullptr NULL
#endif

#include "FTDebouncer.h"

FTDebouncer pinDebouncer(60);


#define goodLED 7
#define badLED 6

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

bool initPassed = false;
bool sendMSG = false;

//#define RH_HAVE_SERIAL

// Singleton instance of the radio driver
RH_NRF24 driver(9,10);  // Works on Arduino UNO/Nano

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void configRadio(){

  //DataRate1Mbps = 0,   ///< 1 Mbps
  //DataRate2Mbps,       ///< 2 Mbps
  //DataRate250kbps      ///< 250 kbps

  // Add 20dBm for nRF24L01p with PA and LNA modules
  //TransmitPowerm18dBm = 0,        ///< On nRF24, -18 dBm
  //TransmitPowerm12dBm,            ///< On nRF24, -12 dBm
  //TransmitPowerm6dBm,             ///< On nRF24, -6 dBm
  //TransmitPower0dBm,              ///< On nRF24, 0 dBm

  driver.setChannel(2); // The default, in case it was set by another app without powering down
  driver.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm);
  //driver.printRegisters();

  delay(2000);
}

void blinkLed(int led, int onTime=100, int offTime = 100, int blinkCnt = 1 ){
  for (int i = 0; i < blinkCnt; i++){
    digitalWrite(led, HIGH);
    delay(onTime);
    digitalWrite(led, LOW);
    delay(onTime);
    }
}

void setup() 
{
  pinDebouncer.addPin(4, LOW);
  
  pinDebouncer.begin();
  Serial.println(sizeof(pinDebouncer));
  Serial.println(pinDebouncer.getPinCount());
 
  pinMode(goodLED,OUTPUT);  // Flash LED on good
  pinMode(badLED,OUTPUT);  // Flash LED on good
  
  Serial.begin(115200);
  initPassed = manager.init();
  
  if (initPassed){
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    configRadio();
  }
  else{
    Serial.println("init failed");
  }
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

void loop()
{
  pinDebouncer.update();
  
  if(sendMSG){
    if(initPassed){
      Serial.println("Sending to nrf24_reliable_datagram_server");
        
      // Send a message to manager_server
      if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
      {
        // Now wait for a reply from the server
        uint8_t len = sizeof(buf);
        uint8_t from;   
        if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
        {
          Serial.print("got reply from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char*)buf);
          blinkLed(goodLED,5,5);
          //sendMSG = false;
        }
        else
        {
          Serial.println("No reply, is nrf24_reliable_datagram_server running?");
        }
      }
      else{
        Serial.println("sendtoWait failed");
        blinkLed(badLED);
      }
      
      delay(2);
    }
    else{
      Serial.println("Init Failed -- Check Radio and reset conroller");
      blinkLed(badLED,50, 50, 20);
      delay(5000);
    }
  }
  
}

void onPinActivated(int pinNr){
  //Serial.print("Pin activated: ");
  //Serial.println(pinNr);
  switch(pinNr){
    case 4:
      if(sendMSG == false){
        sendMSG = true;
        Serial.println("START Sending msgs");
      }
      else{
        sendMSG = false;
        Serial.println("STOP  Sending msgs");
      }
      break;
    default:
        break;
  }
}

void onPinDeactivated(int pinNr){
  //Serial.print("Pin deactivated: ");
  //Serial.println(pinNr);
}