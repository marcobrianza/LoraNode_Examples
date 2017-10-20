
/*
  The example sends every 60 seconds a temperature value on ttn with LoraNode 1.3

  A one Wire DS18B20 temperature sensor is required.
  Pinout looking at the component face is: GND - DATA - VDD

  Connect
  GND to GND
  DATA to D1
  VDD to 3.3V

to decode data in the ttn backend use this payload function in "payload formats" of the ttn console
https://github.com/marcobrianza/_ttn-functions/blob/master/ttn_float_decoder.js


  ---

  by Marco Brianza, Sept 2017

  This example code is in the public domain.
*/

#include <lmic.h>  // use this version of LMIC https://github.com/marcobrianza/arduino-lmic
#include <hal/hal.h>
#include <ESP8266WiFi.h>

// --- Network Configuration -- //

#include "credentials.h"

// --- Radio Configuration --///
u1_t radioPower = 14; // Radio power in dbm (0..20)
int radioChannel = 0; // select a fixed channel (0..7) or a random one (-1)
_dr_eu868_t sf = DR_SF7; // select Spreding Factor (from DR_SF7 to DR_SF12 ) DR_SF7=fast and near, DR_SF12=slow and far


//--- application settings---//

int TX_INTERVAL = 60; //interval of transmission (seconds)
unsigned long last_tx = 0;
unsigned long nunc = 0;

// --------- end Configuration --------||

// --- Data global variables used from TX RX--//
uint8_t txPort = 1; // port for uplink message
static uint8_t txData[52]; // byte array of uplink message
uint8_t txLen = 1; // lenght of the data of uplink message
boolean txACK = false; // if true the backend sends back a message for confirmation

uint8_t rxPort;
static uint8_t rxData[52];

//----------------------//

extern int loraTransmitting;
extern int loraAvailable;


// Temperature definition block
#include <OneWire.h>
#include <DallasTemperature.h>  // DallasTemperature 3.7.6 from Libray manager (https://github.com/milesburton/Arduino-Temperature-Control-Library)
#define ONE_WIRE_BUS D1 // Data wire is plugged into port D1 on the Arduino
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
DeviceAddress thermometer; // variable to hold device address

void setup() {
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  Serial.println("\nttn ABP - LoraNode Temperature - " + nodeid);

  pinMode(RX, INPUT_PULLUP); // this line must be after Serial.begin

  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  blink(3);

  loraBegin();
  setupSensor();
  sendApplicationData() ;
}

int r;
void loop() {
  os_runloop_once();

  switch (loraTransmitting) {
    case 0:
      if  ((digitalRead(RX) == LOW) ) { //manualy send the temperature
        last_tx = millis();
        sendApplicationData() ;
      } else {
        nunc = millis();
        if ((nunc - last_tx) > (TX_INTERVAL * 1000)) {
          last_tx = nunc;
          Serial.print(nunc);
          Serial.println(" time to transmit");
          sendApplicationData() ;
        }
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
}


void sendApplicationData() {

  sensors.requestTemperatures(); // Send the command to get temperatures
  float t = sensors.getTempC(thermometer);
  Serial.println(t);
  txPort = 7;
  txLen = sizeof(t);  //float is 4 bytes
  memcpy(txData, &t, txLen);
  // we send the float raw dato to the backend to avoid overhead. it will be decoded there or in the application endpoint
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



// function to print a device address
String address2Str(DeviceAddress deviceAddress)
{
  String sa;
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) sa = sa + "0";
    sa = sa + String(deviceAddress[i], HEX);
  }
  sa.toUpperCase();
  return sa;
}



void setupSensor() {
  boolean result = false;
  do {
    Serial.println("\nDallas Temperature IC Control Library Demo");

    // locate devices on the bus
    sensors.begin();
    Serial.print("Termometers found: ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println();

    // report parasite power requirements
    Serial.print("Parasite power is: ");
    if (sensors.isParasitePowerMode()) Serial.println("ON");
    else Serial.println("OFF");

    if (sensors.getAddress(thermometer, 0)) {
      String thermometerStr = address2Str(thermometer);
      Serial.println(thermometerStr + " ");
      // set the resolution to 12 bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(thermometer, 12);
      result = true;
    }
    else {
      Serial.println("Unable to find sensor, please check wiring...");
      delay(10000);
    }
  } while (result == false);

}
