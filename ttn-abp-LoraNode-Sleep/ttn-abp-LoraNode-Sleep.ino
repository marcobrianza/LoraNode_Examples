/*
  Example code to send board barttery voltage on ttn with LoraNode 1.3
  every TX_INTERVAL minutes value is sent then after receiving timeout the board goes to sleep

  Fill node credentials in credentials.h you can copy paste form the console.
  press Red button to send the message.
  monitor serial port to see LMIC messages

  by Marco Brianza, July 2018

  This example code is in the public domain.
*/
//ADC_MODE(ADC_VCC);
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
uint8_t txPort = 1; // port for uplink message
static uint8_t txData[52]; // byte array of uplink message
uint8_t txLen = 1; // lenght of the data of uplink message
boolean txACK = false; // if true the backend sends back a message for confirmation

uint8_t rxPort; // port for downlink messaage
static uint8_t rxData[52]; // byte array  of downlink message
//----------------------//

//-- Application---//
extern int loraTransmitting;
extern int loraAvailable;

int TX_INTERVAL = 1; //in minutes
#define BATTERY_PIN A0
#define LED_ON 0
#define LED_OFF 1


void setup() {
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  Serial.println("\nttn ABP - LoraNode Sleep - " + nodeid);

  pinMode(RX, INPUT_PULLUP); // this line must be after Serial.begin

  //pinMode(LED_BUILTIN, OUTPUT);
  //blink(3);

  loraBegin();
  sendApplicationData() ;

}


void loop() {
  os_runloop_once();
  //gotoSleepESP8266(); // if no reception is needed you can sleep here
  
  switch (loraTransmitting) {
    case 0:
//      if  ((digitalRead(RX) == LOW) ) {
//        sendApplicationData() ;
//      }
      break;
    case 1:
     // digitalWrite(LED_BUILTIN, LED_ON);
      break;
    case -1:
     // digitalWrite(LED_BUILTIN, LED_OFF); 
      loraTransmitting = 0;
      gotoSleepESP8266();
      break;
    case -2: //ACK received
     // digitalWrite(LED_BUILTIN, LED_OFF);
      delay(500);
      blink(2);
      loraTransmitting = 0;
      gotoSleepESP8266();
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

} // loop





void sendApplicationData() {
// no need to reset coubter since esp resets evety time
  //reset time limits that are not incremented during sleep;
  //Found it: You can reset the duty cycle limits after sleep by calling:
//  LMIC.bands[BAND_MILLI].avail = os_getTime();
//  LMIC.bands[BAND_CENTI].avail = os_getTime();
//  LMIC.bands[BAND_DECI].avail = os_getTime();

  txPort = 25;
  txACK = false;

  float batteryVoltage = getBatteryVoltage_WemosLipo();

  txLen = sizeof(batteryVoltage);//float is 4 bytes
  Serial.print("batteryVoltage: ");
  Serial.println(batteryVoltage);
  memcpy(&txData[0], &batteryVoltage, txLen);

  sendData();
}


void blink(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(150);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(150);
  }
}


float getBatteryVoltage_WemosLipo() {
  // 130+220+100 =450 / 100
  float adcReading = analogRead(BATTERY_PIN);
  // Discard inaccurate 1st reading
  adcReading = 0;
  // Perform averaging
  for (int counter = 0; counter < 10; counter++)
  {
    adcReading += analogRead(BATTERY_PIN);
  }
  adcReading = adcReading / 10;


  // Convert to volts
  float batteryVoltage = adcReading * (4.5 / 1023.0); //adc is 10 bit resolution

  return batteryVoltage;

}

float getBatteryVoltage_WemosVcc() {

  // ADC_MODE(ADC_VCC); // at beginning of sketch

  float adcReading = ESP.getVcc();
  // Discard inaccurate 1st reading
  adcReading = 0;
  // Perform averaging
  for (int counter = 0; counter < 10; counter++)
  {
    adcReading += ESP.getVcc();
  }
  adcReading = adcReading / 10;

  // Convert to volts (1.118 is to compensate external resistor divider on A0)
  float batteryVoltage = adcReading * (1.118 / 1023.0); //adc is 10 bit resolution
  return batteryVoltage;
}


void gotoSleepESP8266() {
  Serial.println("sleeping\n");
  // convert to microseconds
  unsigned long sleepUs = (TX_INTERVAL * 60 * 1000 - millis()) * 1000;
  Serial.printf("Sleep for %d u_seconds\n\n", sleepUs);
  ESP.deepSleep(sleepUs);
}



