# esp32_ws2812

control a ws2812-lightstrip with an esp32. It works with the RMT driver, designed for remote control interfaces.
Because it has to parallel with a web server it was necessary to set up some special settings.

## hardware 

There's a 74HCT14 as an interface between the 3,3 V esp32 interface and the led strip 5v data input.
A resistor of 470 ohm is recommended in the data line.
A capacitor of 1000 µF is used for stabilisation of the power supply.
[schematics](assets/esp32_ws2812_schematics.png)

## software

A REST service is used for controlling the displayed scenes. 

To bring the wifi interface up the esptouch framework version 1 is used, you need the esptouch app on a smartphone.

### API

#### /config

Stores global parameters, the setting is permanent stored in the ESP32 flash.

true is 't','true' or '1'
false is 'f', 'false' or '0'

type: GET

parameter:

* numleds=number of leds, default 12, a reboot is triggered after changing this parameter
* autoplay=true|false - start immediately after reboot - true if theres no network connection, default: *true*
* autoplayfile=<name> - scenefile for autoplay, default: *scenes*
* cycle=<cycletime> - cycle time in ms, default 100 ms
* showstatus=true|false - the first led on the strip shows the status of the running software

response:

JSON object with the actual settings

#### /reset

Resets all settings in the ESP32 flash, and restart the system.

type: GET

parameter: none

response: "RESET done"

#### /ctrl

controls the status 

type: GET

parameter: 

* cmd=r|s|p|l|c - sets the status
    * r - run
    * s - stop
    * p - pause, continue with 'r'
    * l - list active events
    * c - stopps all events and clear all scene events 
* add=<event specifikation> - adds an event
* del=<event number> - deletes the event with a number
* set=<event specification with event number> - replaces an event with a new one

Response: some message 

