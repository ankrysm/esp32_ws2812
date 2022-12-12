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

the first led in the strip is used for status display

* white - boot up
* yellow - search for WIFI
* green - WIFI connected
* blue - esptouch started
* red - no wifi connection

### objects

this is what to display, may be a solid color, color transition or somethng else.

### events

There are scenes stored in a chained list. Every scene has a list of events.

## API

path - description
* '/r' - run
* '/s' - stop
* '/p' - pause
* '/b' - blank strip
* '/i' - info
* '/cl' - clear event list
* '/li' - list events
* '/lo' - load events, replaces data in memory, requires POST data
* '/f/list' - list stored files
* '/f/store/<fname>' - store JSON event lists into flash memory as <fname>, requires POST data
* '/f/get/<fname>' - get content of stored file <fname>
* '/f/load/<fname>' - load JSON event list stored in <fname> into memory
* '/f/delete/<fname>' - delete file <fname>
* '/cfg/get' - show config
* '/cfg/set' - set config, requires POST data
* '/cfg/restart' - restart the controller
* '/cfg/tabula_rasa' - reset all data to default
* '/' - API help


## JSON attributes for config

| attribute | type | description |
| ---- | ----| ---- |
| "name" | string | name of the controller |
| "numleds" | numeric | number of leds of the strip, default 60 |
| "cycle" | numeric | cycle time in ms, default 50 |


| "show_status" | boolean | if true, shows the status of the system with the first led:<br>white: init,<br> yellow: try to connect<br>, green: connected<br>blue: easy connect in progress,<br>red: no WIFI connection |
| "autoplay" | boolean |if true, play scenes automatically after boot up |
| "autoplay_file" | string |file name to load or play after boot up |
| "timezone" | string | for localtime |
| "extended_log" | numeric | for serial console : 0 - normal logs, 1: more loges, 2: extended logs |

## JSON attributes for display

    +- "objects", type: list, contains a list of objects with following attributes:
    |    +- "id", type: string, id of the data list for reference in display events
    |    +- "list", type: list, contains a list of display objects with following attributes:
    |         +- "type", type:string, type of object, see table
    |         +- "pos", type: numeric, relative position in list
    |         +- "len", type_ numeric, len of the section
    |         +- color specification, type: string, depend from type, see listing
    +- "scenes", type: list, containes objects for events with following attributes
         +- "id", type: string
         +- "events", type: list of events, contains objects with following attributes
               +- "id", type: string
               +- "repeats", type: numerc, number of repeates, 0 = forever
               +- "init", type: list, contains events executed during scene start up with the following attributes
               |    +- "type", type: string, event type, see table
               |    +- "value", tpye: string, numeric
               +- "work", type: list, contains events executed during working time with the following attributes
               |    +- "type", type: string, event type, see table
               |    +- "time", type: numeric, execution time, 0=immediately (default)
               |    +- "value", tpye: string, numeric
               |    +- "marker", type: string, destination for "jump_marker"
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
| ---- | ---- | ---- | ---- | ---- |
| "pause" | - | start, running | - | stop painting for the amount of time |
| "go" | - | running | - | continue painting with the actual parameters |
| "speed" | yes | start | - | set speed in led / s |
| "speedup" | yes | start | - | set acceleration |
| "bounce" | - | end | - | change direction, sets speed=-speed |
| "reverse" | - | end | - | reverses paint direction |
| "goto" | yes | end | - | jump to position |
| "jump_marker" | - | end | - | jump to event with marker |
| "clear" | yes running| yes | clear leds while timer is running |
| "brightness" | yes | start | yes | set brightness range: 0.0 .. 1.0 |
| "brightness_delta" | yes | start | - | set brightness delta per time slot |
| "object" | yes | end | - | sets the object being displayed |

## Display bitmap files

A video is a 2-dimensional object that changes in a time dimension.
Since an LED strip is only one-dimensional, the 2nd dimension of an image can be used as a time dimension.
Since the RAM of the ESP32 is limited, the data must be available in a format that consumes few resources and can be fetched from an external memory.
The choice fell on the BMP format, which has been available since at least Windows 3.0.

see

[here](https://de.wikipedia.org/wiki/Windows_Bitmap)


