# LoRaNode Examples

Examples to use LoRaWan with [LoraNode_1.3](https://github.com/marcobrianza/Lora_Node_1.3) hardware on The Things Network

## ttn-abp-LoraNode-button-serial
Basic example to use LoraNode   
Press the red buttton to send a packet  
monitor LMIC activity with the serial port

## ttn-abp-LoraNode-displasy
Example code to conficure and send a packet on ttn with LoraNode 1.3 + [OLED shield ](https://wiki.wemos.cc/products:d1_mini_shields:oled_shield)   
All the LoRaWan options are configurable: 

* Channel  
* Radio Power
* Spread Factor/Data Rate
* Port
* Data Lenght
* TX data payload
* Acknowledgment


press the black button to select the item  
the red button to change value  
Select "->" and press the Red button to send the message.


## ttn-abp-LoraNode-temperature
This example sends every 60 seconds a temperature value on ttn  
A one Wire DS18B20 temperature sensor is required.  
To temperature decoda data in the ttn backend use this [payload function](https://github.com/marcobrianza/_ttn-functions/blob/master/ttn_float_decoder.js) in the "payload formats" of the [ttn console](https://console.thethingsnetwork.org/). The output json with add the field with the temperature value.


