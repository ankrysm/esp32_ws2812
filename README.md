# esp32_ws2812

control a ws2812-lightstrip with an esp32. It works with the RMT driver, designed for remote control interfaces.
Because it has to parallel with a web server it was necessary to set up some special settings.

## hardware 

There's a 74HCT14 as an interface between the 3,3 V esp32 interface and the led strip 5v data input.
A resistor of 470 ohm is recommended in the data line.
A capacitor of 1000 µF is used for stabilisation of the power supply.
[schematics](assets/esp32_ws2812_schematics.png)

## software

The software basedd on the official ESP-IDF SDK. Pleas follow the instruction on [get started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)

A REST service is used for controlling the displayed scenes. The interface uses JSON data for config and define scenes.

To bring the wifi interface up the esptouch framework version 1 is used, you need the esptouch app on a smartphone.

## API

* **/load** - load events, replaces stored data, uses JSON-POST-data"},
* **/** or **/help** - API help
* **/st** or **/status** - status informations
* **/l** or **/list** - list events
* **/clear** - clear event list
* **/r** or **/run** - run scenes
* **/s** or **/stop** - stop scenes
* **/p** or **/pause** - pause
* **/b** or **/blank** - stop scenes and blank strip
* **/c** or **/config** - shows config, set values: uses JSON-POST-data
* **/save** - save event list specified by fname=<fname> default: 'playlist'
* **/restart** - restart controller
* **/reset** - reset ESP32 to default, erases all data
