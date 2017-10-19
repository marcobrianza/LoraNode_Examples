/*********************************************************************

Example code to map ttn coverage https://ttnmapper.org/

an app running on a mobile phone is need to log message position
https://play.google.com/store/apps/details?id=com.jpmeijers.ttnmapper
https://itunes.apple.com/us/app/ttn-mapper/id1128809850?mt=8 (not working for me at his time)

For complete functionality the WeMos OLED display is required
set the parameters by the ui (the standard ones should be ok)
set '->' to 1 and move around :-)
the DT number (interval of packet transmission) will be x10 by the code. the standard =1 =10s

*********************************************************************/
#include <ESP8266WiFi.h>
#include <lmic.h>
#include <hal/hal.h>
#include <Adafruit_SSD1306.h>
#include <ClickButton.h>

// --- Network Configuration -- //

#include "credentials.h"

// --- Radio Configuration --///
u1_t radioPower = 14; // Radio power in dbm (0..20)
int radioChannel = -1; // select a fixed channel (0..7) or a random one (-1)
_dr_eu868_t sf = DR_SF7; // select Spreding Factor (from DR_SF7 to DR_SF12 ) DR_SF7=fast and near, DR_SF12=slow and far

// --------- end Configuration --------||

//--- application settings---//

int tx_interval = 10; //interval of transmission (seconds)
int autoSend = 0;

unsigned long last_tx = 0;
unsigned long nunc = 0;
int msgCounter = 0;


// --- Data Variables --//
uint8_t txPort;
static uint8_t txData[52]; //51 bytes size is the max I transmitted and received
uint8_t txLen = 0;
boolean txACK = true;

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
  {"CH ", 0, 1, -1, 7 , -1, DEC},
  {"PW ", 5, 1, 0, 20 , 14, DEC},
  {"SF ", 0, 2, 6, 12 , 7, DEC},
  {"PO ", 5, 2, 1, 99 , 42, DEC},
  {"AK ", 0, 3, 0, 1 , 0, DEC},
  {"DT ", 5, 3, 1, 9 , 1, DEC},
  {"MC ", 0, 4, 0, 999 , 0, DEC},
  {"->", 7, 4, 0, 1 , 0, DEC},
};


#define CHANNEL 0
#define POWER 1
#define SPREADF 2
#define PORT 3
#define ACK 4
#define DELTA_T 5
#define MC 6
#define SEND 7

int b = SEND;

void setup()   {
  WiFi.mode(WIFI_OFF);

  //  Serial.begin(115200);
  //  Serial.println("\nttn ABP - LoraNode mapper - " + nodeid);
  //  pinMode(RX, INPUT_PULLUP);


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

  os_runloop_once();

  switch (loraTransmitting) {
    case 0:
      handleButtons();

      nunc = millis();
      if ((nunc - last_tx) > (tx_interval * 1000)) {
        last_tx = nunc;
        if ( autoSend) {
          SendApplicationData();
        }
      }
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
  txACK = ui[ACK].value;

  tx_interval = ui[DELTA_T].value * 10;

  autoSend = ui[SEND].value;

  drawUI();

  if (radioChanged) {
    blink(1);
    loraBegin();
  }


}

void SendApplicationData() {
  ui[MC].value++;
  txLen = 8;
  for (int i = 0; i < txLen; i++) {
    txData[i] = 0xAB;
  }
  sendData();
}


void setCursorChar(int x, int y) {
  //for text size=1
  display.setCursor(x * 6, y * 8);
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


void blink(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
  }
}
