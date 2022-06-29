# esp32_ws2812
control a ws2812-lightstrip with an wsp32

the plan to use a dastLED-Library was not successful because of timing troubles when integrating a web server.

I've found some ws2812 code in the esp-idf examples. This works parallel with the http server.

## newer concepts

### API

base path is */api/v1*


#### config

stores global parameters

true is 't','true' or '1'
false is 'f', 'false' or '0'

type: GET

Parameter:

* autoplay true|false - start immediately after reboot - true if theres no network connection, default: *true*
* scenefile <name> - scenefile for play, default: *scenes*
* repeat true|false - plays scenes once or forever, default *true*
* numleds - number of leds, default 12
* cycle - cycle time in ms, default 100 ms

Response:

actual key-value-list of parameters

### status / run / pause /stop

sets or gets the status of played scenes

type: GET

Parameter:

* scenefile <name> - optional, new scenefile name for play

Response:

* actual status: run/pause/stop
* cycle count
* elapsed time in seconds
* actual scene file

### scene

show a single scene immediately, implies status stop

type: POST

parameter: none

POST-data: scene definition

### scenes

upload of a scene data and storage in a file on the controller.
A scene file contains a list of scenes in JSON-format

type POST

parameter:

* file name - used name for storage

POST-data: JSON-Structure for list of scenes

Response: OK/NOK

Reason for NOK may be
* format errors
* not enough storage space
* syntax errors


## scene definitions

### common parameter

a scene definition consists of
* common parameters
    * "start":xx.yy - start time in seconds - scene will be activated xxx.yy seconds after cycle start - default 0
    * "duration":xx.yy - duration of a scene in seconds - default 0 - means forever
    * "pos":nn - start position - default 0
    * "len":nn - number of leds affected by this scene - default number leds
    * "type":"name" - name of the scene type
    * "bg_color":{COLOR} - background color - default black
    * "fadein"
        * "t":sss.ss - fade in time in seconds - default 0
        * "in":nn - fade in length (number of leds) - default 0
        * "out":nn - fade out length - default 0
    * "movement"
        * "speed":nn - may be negative or positive, value = led steps per second
        * "rotate":"true"|"false" - rotate or shift content, if content shift out, freed leds will be set to bg_color

* special parameters - depends from the scene type

Colors where specified as `"rgb":[rrr,ggg,bbb]` or `"hsv":[hhh,sss,vvv]` or `"color":"name"`. In this document mentions as *COLOR*

### scene types

#### blank

description: all leds are switched off (set to black)

parameters: none

#### solid

description: sets the range to specific color

parameters:

* "color"={COLOR}

#### blinking

description: color transition from *bg_color* to *color* and back

parameters:
* "color":{COLOR} - to color
* ttime=nn - transition time in sec - default 0
* ctime=nn - cycle time in sec - default 1.0 sec


#### rainbow

description: display rainbow colors

parameters:
* "min_brightness":nn - default 0%
* "max_brightness":nn - default 100%
* "cycles":nn - default 1


#### sparkle

description: shows some sparkling elements

paramter:
* "bg_color":{COLOR} - background color - default black
* "colors":[{COLOR},..] - List of foreground colors
* "size":n - n=0(small)..5(big) - size of a sparkle, default 3
* "vary":n - n=0(few}..5(a lot) - varying of foreground color, default 3
* "spped":n - n=0..10 - speed index
* "num":n - number of sparkles

#### move

description: moves scenes

parameters: none


### examples

```
{
  "start":1.5,"duration":3.5,"pos":0,"len":50,"type":"blinking",
  "bg_color":{"color":"black"},"color":{"rgb":[128,255,128]},"ttime":0.25,"ctime":1.0
}

{
  "start":1.5,"duration":3.5,"pos":0,"len":50,"type":"sparkle",
  "bg_color":{"rgb":[255,0,0]},"colors":[{"rgb":[0,255,0]},{"hsv":[34,55,100]}],"ttime":0.25,"ctime":1.0
}

```

