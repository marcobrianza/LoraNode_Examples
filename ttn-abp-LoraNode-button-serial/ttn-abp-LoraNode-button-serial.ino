/*
Example code to send a packet on ttn with LoraNode 1.3

Fill node credentials in credentials.h you can copy paste form the console.
press Red button to send the message.
monitor serial port to see LMIC messages

by Marco Brianza, Sept 2017

This example code is in the public domain.
*/

#include <lmic.h>  // use this version of LMIC https://github.com/marcobrianza/arduino-lmic
#include <hal/hal.h>
#include <ESP8266WiFi.h>

// --- Network Configuration -- //

#include "credentials.h"  /

// --- Radio Configuration ---///
u1_t radioPower = 14; // Radio power in dbm (0..20)
int radioChannel = -1; // select a fixed channel (0..7) or a random one (-1)
_dr_eu868_t sf = DR_SF7; // select Spreding Factor (from DR_SF7B to DR_SF12 ) DR_SF7B=fast and near, DR_SF12=slow and far


// --- end Configuration ---||


// --- Data global variables used from TX RX--//
uint8_t txPort=1;   // port for uplink message
static uint8_t txData[52]; // byte array of uplink message
uint8_t txLen = 1; // lenght of the data of uplink message
boolean txACK = false; // if true the backend sends back a message for confirmation

uint8_t rxPort; // port for downlink messaage
static uint8_t rxData[52]; // byte array  of downlink message
//----------------------//

extern int loraTransmitting;
extern int loraAvailable;

void setup() {
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  Serial.println("\nttn ABP - LoraNode button - " + nodeid);

  pinMode(RX, INPUT_PULLUP); // this line must be after Serial.begin

  pinMode(LED_BUILTIN, OUTPUT);
  blink(3);

  loraBegin();
}


void loop() {

  switch (loraTransmitting) {
    case 0:
      if  ((digitalRead(RX) == LOW) ) {
        sendApplicationData() ;
      }
      break;
    case 1:
      digitalWrite(LED_BUILTIN, LOW); //turn led ON
      break;
    case -1:
      digitalWrite(LED_BUILTIN, HIGH); //turn led OFF
      loraTransmitting = 0;
      break;
    case -2: //ACK received
      digitalWrite(LED_BUILTIN, HIGH); //turn led OFF
      delay(500);
      blink(2);
      loraTransmitting = 0;
      break;
  }


  if (loraAvailable > 0) {
    Serial.println("rx bytes: " + String(loraAvailable));
    Serial.println("rxPort: " + String (rxPort));
    String hexByte = "";
    String hexData = "";
    for (int i = 0; i < loraAvailable; i++) {
      hexByte = String(rxData[i], HEX);
      hexByte.toUpperCase();
      hexData = hexData + hexByte + " ";
    }
    Serial.println("rxData: " + hexData);
    delay(500);
    blink(loraAvailable);
    loraAvailable = -1;
  }


  os_runloop_once();
}



void sendApplicationData() {
  int TX_SIZE = 8;
  txPort = 42;
  txLen = TX_SIZE;

  for (int i = 0; i < TX_SIZE; i++) {
    txData[i] = i ;
  }
  sendData();
}



void blink(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
  }
}




