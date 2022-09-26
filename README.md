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
* **/lf** or **/list_files** - list stored files
* **/clear** - clear event list
* **/r** or **/run** - run scenes
* **/s** or **/stop** - stop scenes
* **/p** or **/pause** - pause
* **/b** or **/blank** - stop scenes and blank strip
* **/c** or **/config** - shows config, set values: uses JSON-POST-data
* **/restart** - restart controller
* **/reset** - reset ESP32 to default, erases all data

## JSON attributes

### attribute "filename"

description: file name to store JSON data in the ESP32 flash<br>
type: string

### "objects"

description: list with display objects<br>
type: list

**object list entries:**

description: contains objects with display data<br>
type: object

* **"id"** <br>
description: id of the data list for reference in display events<br>
type: string

* **"list"**<br>
description: list of **display objects**<br>
type: object


#### display object

contains data for what to display with the following attributes:

* **"type"**<br>
description: type of object<br>
type: string

* **"pos"**<br>
description: relative position in list<br>
type: numeric

* **"len"**<br>
description: len of the section<br>
type: numeric

* color specification<br>
description: colors are specified by various attributs<br>
type "string"
     * **"color"** - color by name
     * **"rgb"** - color as "r,g,b" tripel
     * **"hsv"** - color as "h,s,v" tripel
     * **"color_from"**, **"rgb_from"**, **"hsv_from"**, **"color_to"**, **"rgb_to"**, **"hsv_to"** - special attributes for color transitions


| **"type"** attribute | description |
| ---- | ---- |
| "color" | a single color |
| "color_transition" | color transition *..._from* to *..._to* |



