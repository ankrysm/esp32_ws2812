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

		+- "filename", type: string, contains a file name to store JSON data in the ESP32 flash.
		+- "objects", type: list, contains a list of objects with following attributes:
      	|	+- "id", type: string, id of the data list for reference in display events
      	|	+- "list", type: list, contains a list of display objects with following attributes:
         |  		+- "type", type:string, type of object, see table
         |   		+- "pos", type: numeric, relative position in list
         |   		+- "len", type_ numeric, len of the section
         |   		+- color specification, type: string, depend from type, see listing
		+- "scenes", type: list, containes objects for events with following attributes
			+- "id", type: string
			+- "events", type: list of events, contains objects with following attributes
				+- "id", type: string
				+- "repeats", type: numerc, number of repeates, 0 = forever
				+- "init", type: list, contains events executed during scene start up with the following attributes
				|	+- "type", type: string, event type, see table 
				|	+- "value", tpye: string, numeric
				+- "work", type: list, contains events executed during working time with the following attributes
				|	+- "type", type: string, event type, see table 
				|	+- "time", type: numeric, execution time, 0=immediately (default) 
				|	+- "value", tpye: string, numeric
				+- "final", type: list, contains objects with the following attributes
					+- "type", type: string, event type, see table 
					+- "value", tpye: string, numeric

### color attributes
* **"color"** - color by name
* **"rgb"** - color as "r,g,b" values
* **"hsv"** - color as "h,s,v" values
* **"color_from"**, **"rgb_from"**, **"hsv_from"**, **"color_to"**, **"rgb_to"**, **"hsv_to"** - special attributes for color transitions


### object types
| **"type"** attribute | description |
| ---- | ---- |
| "color" | a single color |
| "color_transition" | color transition *..._from* to *..._to* |
| "rainbow"  | show rainbow colors |

### event types

for init-, work- or final events

| **"type"** attribute | in *init*| in *work* | in *final* | description |
| ---- | ---- |
| "pause" | - | start, running | - | stop painting for the amount of time |
| "go" | - | running | - | continue painting with the actual parameters |
|

description


