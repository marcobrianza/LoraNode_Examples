/*
  Example code to conficure and send a packet on ttn with LoraNode 1.3 + Oled display

  Fill node credentials in credentials.h you can copy paste form the console.

  All the LoraWan options are configurable: press black button to select item, Red button to change value
  Select "->" and press Red button to send the message.

  by Marco Brianza, Sept 2017

  This example code is in the public domain.
*/


#include <ESP8266WiFi.h>
#include <lmic.h> // use this version of LMIC https://github.com/marcobrianza/arduino-lmic
#include <hal/hal.h>
#include <ClickButton.h> // https://github.com/marcobrianza/ClickButton
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  //use this version https://github.com/marcobrianza/Adafruit_SSD1306/tree/esp8266-64x48


// --- Network Configuration -- //

#include "credentials.h"

// --- Radio Configuration --///
u1_t radioPower = 14; // Radio power in dbm (0..20)
int radioChannel = 0; // select a fixed channel (0..7) or a random one (-1)
_dr_eu868_t sf = DR_SF7; // select Spreding Factor (from DR_SF7 to DR_SF12 ) DR_SF7=fast and near, DR_SF12=slow and far


// --------- end Configuration --------||


// --- Data global variables used from TX RX--//
uint8_t txPort=1;   // port for uplink message
static uint8_t txData[52]; // byte array of uplink message
uint8_t txLen = 1; // lenght of the data of uplink message
boolean txACK = false; // if true the backend sends back a message for confirmation

uint8_t rxPort;
static uint8_t rxData[52];
//----------------------//

extern int loraTransmitting;
extern int loraAvailable;

//--- Display and buttons

Adafruit_SSD1306 display(10); //10 is a dummy GPIO
#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

ClickButton buttonB(TX, LOW, CLICKBTN_PULLUP);
ClickButton buttonR(RX, LOW, CLICKBTN_PULLUP);

//--- user interface setup ---
typedef struct
{
  String label;
  int x;
  int y;
  int min;
  int max;
  int value;
  int format;
}  ui_type;

#define MAX_UI 8

ui_type ui[MAX_UI] = {
  {"CH ", 0, 1, -1, 7 , 0, DEC},
  {"PW ", 5, 1, 0, 20 , 14, DEC},
  {"SF ", 0, 2, 6, 12 , 7, DEC},
  {"PO ", 5, 2, 1, 99 , 1, DEC},
  {"DL ", 0, 3, 0, 51 , 1, DEC},
  {"TX ", 5, 3, 0, 255 , 0xAB, HEX},
  {"AK ", 0, 4, 0, 1 , 0, DEC},
  {"-> ", 5, 4, 0, 1 , 0, DEC},
};

#define CHANNEL 0
#define POWER 1
#define SPREADF 2
#define PORT 3
#define TXL 4
#define TXD 5
#define ACK 6
#define SEND 7


int b = SEND;

void setup()   {
  WiFi.mode(WIFI_OFF);

  pinMode(RX, INPUT_PULLUP);
  pinMode(TX, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); //turn led OFF

  buttonB.debounceTime   = 20;   // Debounce timer in ms
  buttonB.multiclickTime = 150;  // Time limit for multi clicks
  buttonB.longClickTime  = 2000; // time until "held-down clicks" register

  buttonR.debounceTime   = 20;   // Debounce timer in ms
  buttonR.multiclickTime = 150;  // Time limit for multi clicks
  buttonR.longClickTime  = 2000; // time until "held-down clicks" register

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.setTextSize(1); //10x6
  drawUI();

  loraBegin();
  updateUiValues();

}


void loop() {

  switch (loraTransmitting) {
    case 0:
      handleButtons();
      break;
    case 1:
      digitalWrite(LED_BUILTIN, LOW); //turn led ON
      break;
    case -1:
      digitalWrite(LED_BUILTIN, HIGH); //turn led OFF
      drawUI();
      loraTransmitting = 0;
      break;
    case -2: //ACK received
      digitalWrite(LED_BUILTIN, HIGH); //turn led OFF
      delay(500);
      blink(2);
      drawUI();
      loraTransmitting = 0;
      break;
  }

  if (loraAvailable > 0) {
    displayReceived();
    delay(500);
    blink(loraAvailable );
    loraAvailable = -1;
  }

  os_runloop_once();
}

void displayReceived() {

  setCursorChar(0, 5);
  display.setTextColor(WHITE);
  display.print(loraAvailable);
  display.print(" ");
  display.print(rxPort);
  display.print(" ");
  for (int i = 0; i < loraAvailable; i++) {
    display.print(rxData[i], HEX);
  }
  display.display();
}


void drawUI() {
  display.clearDisplay();

  setCursorChar(0, 0);
  display.setTextColor( WHITE);
  display.print(nodeid);

  for (int i = 0; i < MAX_UI; i++) {
    setCursorChar(ui[i].x, ui[i].y);
    if (i == b)display.setTextColor(BLACK, WHITE);
    else   display.setTextColor( WHITE);

    display.print(ui[i].label);
    display.print(ui[i].value, ui[i].format);
  }
  display.display();
}


void updateUiValues() {
  bool radioChanged = false;

  if (radioChannel !=  ui[CHANNEL].value) {
    radioChannel = ui[CHANNEL].value;
    radioChanged = true;
  }

  if (radioPower !=  ui[POWER].value) {
    radioPower = ui[POWER].value;
    radioChanged = true;
  }

  if (sf !=  (12 - ui[SPREADF].value)) {
    sf = (_dr_eu868_t)(12 - ui[SPREADF].value);
    radioChanged = true;
  }

  txPort = ui[PORT].value;

  txLen = ui[TXL].value;

  txACK = ui[ACK].value;

  drawUI();

  if (radioChanged) {
    blink(1);
    loraBegin();
  }

  if (ui[SEND].value == 1) {
    ui[SEND].value = 0;
    sendApplicationData();
  }
}



void sendApplicationData() {
  for (int i = 0; i < txLen; i++) {
    txData[i] = ui[TXD].value;
  }
  sendData();
}



void handleButtons() {
  buttonR.Update();
  buttonB.Update();

  if (buttonB.clicks == 1 ) {
    b = (b + 1) % MAX_UI;
    updateUiValues();
  }

  if (buttonR.clicks == 1 ) { //short press
    ui[b].value++;
    if (ui[b].value > ui[b].max) ui[b].value = ui[b].min;
    updateUiValues();
  }

  if (buttonR.clicks == -1 ) { //long press
    ui[b].value--;
    if (ui[b].value < ui[b].min) ui[b].value = ui[b].max;
    updateUiValues();
  }

}


void setCursorChar(int x, int y) {
  //for text size=1
  display.setCursor(x * 6, y * 8);
}



void blink(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
  }
}








