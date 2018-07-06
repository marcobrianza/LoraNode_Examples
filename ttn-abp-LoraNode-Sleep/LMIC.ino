   /* Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

   Permission is hereby granted, free of charge, to anyone
   obtaining a copy of this document and accompanying files,
   to do whatever they want with them without any restriction,
   including, but not limited to, copying, modification and redistribution.
   NO WARRANTY OF ANY KIND IS PROVIDED.

   This example sends a valid LoRaWAN packet with payload "Hello,
   world!", using frequency and encryption settings matching those of
   the The Things Network.

   This uses ABP (Activation-by-personalisation), where a DevAddr and
   Session keys are preconfigured (unlike OTAA, where a DevEUI and
   application key is configured, while the DevAddr and session keys are
   assigned/generated in the over-the-air-activation procedure).

   Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
   g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
   violated by this sketch when left running for longer)!

   To use this sketch, first register your application and device with
   the things network, to set or generate a DevAddr, NwkSKey and
   AppSKey. Each device should have their own unique values for these
   fields.

   Do not forget to define the radio type correctly in config.h.

 *******************************************************************************/
bool DEBUG_LORA = 1;  // set DEBUG_LORA =1 to get LMIC debug messages in serial port

u1_t NWKSKEY[16];
u1_t  APPSKEY[16];
u4_t DEVADDR  ;
byte DEVADDRA[4];

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

int loraTransmitting = 0; // 0=not transmitting, 1=transmitting, -1=just finished, -2 finished with ACK
int loraAvailable = -1;


// Pin mapping for LoraNode 1.3
const lmic_pinmap lmic_pins = {
  .nss = D3,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN,
  .dio = {D8, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
};


void loraBegin() {
  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  convertAddresses();
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

#if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  // NA-US channels 0-71 are configured automatically
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.
#elif defined(CFG_us915)
  // NA-US channels 0-71 are configured automatically
  // but only one group of 8 should (a subband) should be active
  // TTN recommends the second sub band, 1 in a zero based count.
  // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
  LMIC_selectSubBand(1);
#endif

  //*** disable channels to use the single channel gateway on 868.1 and  disable dataRate Adaptation
  if (radioChannel >= 0) {
    for (int i = 0; i < 8; i++) {
      if (i != radioChannel)   LMIC_disableChannel(i);
    }
  }

  //Enable or disable data rate adaptation. Should be turned off if the device is mobile.
  LMIC_setAdrMode(0);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(sf, radioPower); // default was 14

}

void onEvent (ev_t ev) {
  String eventType;
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      eventType = "EV_SCAN_TIMEOUT";
      break;
    case EV_BEACON_FOUND:
      eventType = "EV_BEACON_FOUND";
      break;
    case EV_BEACON_MISSED:
      eventType = "EV_BEACON_MISSED";
      break;
    case EV_BEACON_TRACKED:
      eventType = "EV_BEACON_TRACKED";
      break;
    case EV_JOINING:
      eventType = "EV_JOINING";
      break;
    case EV_JOINED:
      eventType = "EV_JOINED";
      break;
    case EV_RFU1:
      eventType = "EV_RFU1";
      break;
    case EV_JOIN_FAILED:
      eventType = "EV_JOIN_FAILED";
      break;
    case EV_REJOIN_FAILED:
      eventType = "EV_REJOIN_FAILED";
      break;
    case EV_TXCOMPLETE:
    
      if (LMIC.txrxFlags & TXRX_ACK)  {
        eventType = "TXRX_ACK";
        loraTransmitting = -2;
      } else  {
        eventType = "EV_TXCOMPLETE (includes waiting for RX windows)";
        loraTransmitting = -1;
      }

      if (LMIC.dataLen) {
        loraAvailable = LMIC.dataLen;
        rxPort = LMIC.frame[LMIC.dataBeg - 1];
        for (int i = 0; i < loraAvailable; i++) {
          rxData[i] = LMIC.frame[LMIC.dataBeg + i];
        }
      }
      break;

    case EV_LOST_TSYNC:
      eventType = "EV_LOST_TSYNC";
      break;
    case EV_RESET:
      eventType = "EV_RESET";
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      eventType = "EV_RXCOMPLETE";
      break;
    case EV_LINK_DEAD:
      eventType = "EV_LINK_DEAD";
      break;
    case EV_LINK_ALIVE:
      eventType = "EV_LINK_ALIVE";
      break;
    default:
      eventType = "Unknown event";
      break;
  }
  LoraDebug(eventType);
}


void sendData() {
  loraTransmitting = 1;
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    LoraDebug("OP_TXRXPEND, not sending");
  } else {
    // Prepare upstream data transmission at the next possible time.
    if (txLen > sizeof(txData) - 1) txLen = sizeof(txData) - 1;
    LMIC_setTxData2(txPort, txData, txLen, txACK);
    LoraDebug("Packet queued");
  }
}


void LoraDebug(String msg) {
  if (DEBUG_LORA) {
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.println(msg);
  }
}

void convertAddresses() {
  int j;
  char temp[3];

  j = 0;
  for (int i = 0; i < 32; i += 2)
  {
    strncpy(temp, &nwkSKey[i], 2);
    // Convert hex string to numeric:
    NWKSKEY[j] = (temp[0] <= '9') ? (temp[0] - '0') : (temp[0] - 'A' + 10);
    NWKSKEY[j] *= 16;
    NWKSKEY[j] += (temp[1] <= '9') ? (temp[1] - '0') : (temp[1] - 'A' + 10);
    j++;
  }

  j = 0;
  for (int i = 0; i < 32; i += 2)
  {
    strncpy(temp, &appSKey[i], 2);
    // Convert hex string to numeric:
    APPSKEY[j] = (temp[0] <= '9') ? (temp[0] - '0') : (temp[0] - 'A' + 10);
    APPSKEY[j] *= 16;
    APPSKEY[j] += (temp[1] <= '9') ? (temp[1] - '0') : (temp[1] - 'A' + 10);
    j++;
  }

  j = 0;
  for (int i = 0; i < 8; i += 2)
  {
    strncpy(temp, &devAddr[i], 2);
    // Convert hex string to numeric:
    DEVADDRA[j] = (temp[0] <= '9') ? (temp[0] - '0') : (temp[0] - 'A' + 10);
    DEVADDRA[j] *= 16;
    DEVADDRA[j] += (temp[1] <= '9') ? (temp[1] - '0') : (temp[1] - 'A' + 10);
    j++;
  }
  DEVADDR = DEVADDRA[3] | (DEVADDRA[2] << 8) | (DEVADDRA[1] << 16)  | (DEVADDRA[0] << 24);

}

