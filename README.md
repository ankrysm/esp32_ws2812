# esp32_ws2812

## Control a ws2812-lightstrip with an esp32.

It uses the RMT driver, designed for remote control interfaces.

It uses a WIFI connection to apply a web service to control what is displayed on the led strip and a web client to get data from the web. The web client can use https connections, so there's a synchronization with a time server.

## hardware

There's a 74HCT14 as an interface between the 3,3 V esp32 interface and the led strip 5v data input.
A resistor of 470 ohm is recommended in the data line.
A capacitor of 1000 µF is used for stabilisation of the power supply.
[schematics](assets/esp32_ws2812_schematics.png)

## software

The software basedd on the official ESP-IDF SDK. Pleas follow the instruction on [get started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)

A REST service is used for controlling the displayed scenes. The interface uses JSON data for config and define scenes.

To bring the wifi interface up the **esptouch** framework version 1 is used, you need the esptouch app on a smartphone.

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
* '/err' - list last errors
* '/clerr' - clear last errors
* '/lo' - load events, replaces data in memory, requires POST data
* '/f/list' - list stored files
* '/f/store/<fname>' - store JSON event lists into flash memory as <fname>, requires POST data
* '/f/get/&lt;fname&gt;' - get content of stored file <fname>
* '/f/load/&lt;fname&gt;' - load JSON event list stored in <fname> into memory
* '/f/delete/&lt;fname&gt; - delete file <fname>
* '/cfg/get' - show config
* '/cfg/set' - set config, requires POST data
* '/cfg/restart' - restart the controller
* '/cfg/ota/check' - check for new firmware
* '/cfg/ota/update' - ota update
* '/cfg/ota/status' - ota status
* '/cfg/tabula_rasa' - reset all data to default
* '/test/&lt;parameter&gt;' - set leds to color, values via path  &lt;r&gt;/&lt;g&gt;/&lt;b&gt;/[len]/[pos]
* '/help' - API help


## JSON attributes for config

The set configuration function uses POST-data  in JSON_format with the following attributes:

| attribute | type | description |
| ---- | ----| ---- |
| "name" | string | name of the controller |
| "numleds" | numeric | number of leds of the strip, default 60 |
| "cycle" | numeric | cycle time in ms, default 50 |
| "show_status" | boolean | if true, shows the status of the system with the first led:<br>white: init<br>yellow: try to connect<br>green: connected<br>blue: easy connect in progress<br>red: no WIFI connection |
| "autoplay" | boolean |if true, play scenes automatically after boot up |
| "autoplay_file" | string |file name to load or play after boot up |
| "timezone" | string | for localtime, for example 'CET-1CEST,M3.5.0,M10.5.0/3' |
| "extended_log" | numeric | for serial console : 0 - normal logs, 1: more loges, 2: extended logs |
| "ota_url" | string | URL for new firmware versions, the expected binary is <br>esp32_ws2812-Application.bin |


## JSON attributes for display

To display scenes on the led strip you have to define

* "objects" - what to display
* "events" - how to display
* "tracks" - when to show events on objects

here's an example

      {
        "objects" : [
          {  "id":"o1",
            "list" :[
              {"type":"color_transition", "color_from":"blue", "color_to":"red", "pos":0, "len":7},
              {"type":"color", "color":"red", "pos":7, "len":7},
              {"type":"color_transition", "color_from":"red", "color_to":"blue", "pos":14, "len":7}
            ]
          },
          { "id":"rb", "list":[ {"type":"rainbow", "len":50}]}
        ],
        "events": [
          {
          "remarks":"this is an event group",
          "id":"A",
          "init": [
            {"type":"brightness", "value":0.1},
            {"type":"goto", "value":10},
            {"type":"speed", "value":1},
            {"type":"speedup", "value":0.25},
            {"type":"object","value":"rb"}
          ],
            "work": [
                {"type":"distance", "value":290}
          ],
          "final":[
            {"type":"clear"}
          ]
          },
          {"id":"PAUSE","work":[{"type":"pause"}]}
        ],
        "tracks" : [
          {"id":1, "elements":[
            {"event":"PAUSE"},
            {"event":"A", "repeat": 20}
          ] }
        ]
      }


### Attribute "objects"

The "objects" is a list of data structures whith the following attributes:

* "id" - to name and referencing it
* "list" - array with the object components with the attributes
  * "type" - type of the component - mandatory
  * "len" - if needed, initial number of leds affected by the "type"
  * "color" - what color
  * "color_from", "color_to" - start- and end-color in a color transition

**Object component attribute "type":**

Implememted types are

| "type"-attribute | description | additional attributes |
| ---- | ---- | -----| 
| "color" | constant color in range | - "len"<br>  - a color |
| "clear" | synonym for `{"type":"color", "color":"black"}`| - "len"
| "color_transition" | color transistion | - color from as `..._from`<br>- color to as `..._to` |	
| "rainbow" | a rainbow color transition | - "len" |
| "bmp" | a bitmap file from a web site | - "len", default -1: all lines<br>- "url" - where to read the BMP file |

**Color specifications:**

Colors can be specified as 

| color attribute | description |
| --- | --- |
| "color":"&lt;name&gt;" | a named color "red", "blue", "green" ... |
| "hsv":"&lt;h&gt;,&lt;s&gt;,&lt;v&gt;" | color as HSV, H: 0 .. 360, S,V: 0 .. 100 |
| "rgb":"&lt;r&gt;,&lt;g&gt;,&lt;b&gt;" | color as RGB, R,G,B: 0 ..255 |

for a color transition add the suffix "_from" and "_to" to the color attribute

### Attribute "events"

The "event" attribute is a list of objects with the attributes

* "id" - for name and reference it
* "init" - list of steps executed before start displaying
* "work" - list of steps executed during displaying
* "final" - list of steps executed after displaying

Each list of steps can containe these attributes:

| attribute | value | description | init | work | final |
| ---       | ---   | ---         | ---  | ---  | ---   |
| "wait"    | wait time in ms | wait for n ms | &#9745; | &#9745; | &#9744;|
| "wait_first" | wait time in ms | wait for n ms at first init | &#9745; | &#9744; | &#9744;|
| "paint" | execution time in ms | paint leds with the given parameter| &#9744; | &#9745; | &#9744;|
| "distance" | number of leds | paint until object has moved n leds | &#9744; | &#9745; | &#9744;|
| "speed" |speed in leds per second | move with given speed | &#9745; | &#9745; | &#9744;|                       
| "speedup"| delta speed per display cycle | change speed | &#9745; | &#9745; | &#9744;|
| "bounce" | | reverse speed| &#9744; | &#9745; | &#9744;|
| "reverse" | | reverse paint direction| &#9744; | &#9745; | &#9744;|
| "goto" | new led position | go to led position| &#9745; | &#9745; | &#9744;|
| "clear" | | blank the strip| &#9745; | &#9745; | &#9745;|
| "brightness" | brightness factor 0.0 .. 1.0| set brightness| &#9745; | &#9745; | &#9744;|
| "brightness_delta"|brightness delta per display cycle| change brightness| &#9745; | &#9745; | &#9744;|
| "object" | object id string | set object to display from object table| &#9745; | &#9745; | &#9744;|
| "bmp_open" | | open BMP stream, defined by 'bmp' object| &#9745; | &#9745; | &#9744;|
| "bmp_read" |execution time in ms, -1 until end (default) | read BMP data line by line and display it| &#9744; | &#9745; | &#9744;|
| "bmp_close" | |close BMP stream| &#9744; | &#9745; | &#9745;|
| "treshold" | treshold 0 .. 255 | ignore pixel when r- ,g- and b-value is lower than treshold| &#9745; | &#9745; | &#9744;|
| "pause" || "pause display, wait for continue request| &#9745; | &#9745; | &#9744;|

### Attribute "tracks" 

You can define up 16 tracks as a list of evnets to display. The single track has the following attributes:

* "id" - a numeric track number 
* "elements" - a list of events, each element has the attributes
  * "event" - the id of the event that should be processed
  * "repeat" - how many times the event should be executed, optional, default is 1

## Display bitmap files

### Summary
A video is a 2-dimensional object that changes in a time dimension.
Since an LED strip is only one-dimensional, the 2nd dimension of an image can be used as a time dimension.
Since the RAM of the ESP32 is limited, the data must be available in a format that consumes only few resources and can be fetched from an external memory.
The choice fell on the BMP format, which has been available since at least Windows 3.0.

see [here](https://de.wikipedia.org/wiki/Windows_Bitmap)

Because the open of a BMP file lasts about a few seconds you should configure a *wait*-event between execution of the *bmp_open* and the *bmp_read* step.

## Hints for creation BMP files

Because complex scenes consumes a lot of memory the ESP32 application uses the old windows BMP-format to stream pixel data from a web storage.

If you have led strip with 300 leds you need a bmp file with a width of 300. There will be on pixel line displayed in one cycle so the height multiplied by the cycle time determines the display time.

Here are some hints for mac users, may be also relevant to unix users, MS Windows users may use other tools.

**Create a BMP file with MacOS-preview app**

To create BMP files you can use the preview app. To save images as BMP you have to press the option key while selecting entries from the format list box. BMP is not shown normally

**Create a BMP file with** *sips*

Another possibility is to use sips from the command line in the terminal app. It comes with the macOS

Example: `sips -s format bmp -r 90 image.jpeg --out image.bmp`

It converts the file *image.jpg* into *image.bmp* and rotates it by 90 degrees.

The files created by sips are smaller than the files exported by preview app because sips uses 3 bytes per pixel preview uses 4 bytes

**Create a BMP-file using ImageMagick**

To create the image itself you can use *gimp*. It produces *xcf*-files.

ImageMagick is a free tool to manipulate images. On macOS you need homebrew to install ImageMagick. In a terminal session type:

`brew install imagemagick`

If ImageMagick is installed you can convert the xcf files into bmp files. For example

`convert example.xcf -define bmp:format=bmp example.bmp`

Normally you should have to use the parameter `format=bmp3` but it doesn't work in any cases. So because there are many bmp formats you should check it:

`file example.bmp`

If it returns "PC bitmap, Windows 98/2000 and newer format" you have to convert it once again.

`convert example.bmp -define bmp:format=bmp3 example3.bmp`

`file example3.bmp` should return "PC bitmap, Windows 3.x format". This is the format for our purpose.

There are two methods to store pixels in a bmp file: up to down or down to up. If it doesn't fit use the additional convert option `-flip`



