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

---------


## old Concepts

Objects are stored in files on the spiff.
The entries are lines in a specific format.
The lines are list with comma separated values. they start with an index number to identify the line

Color values are in RGB-format as 'R,G,B' or in HSV-Format as 'H,S,V' and has always 3 values.

TODO: named colors

### color
color of a pixel, formats are RGB or HSV
- solid
- random
- as list of colors


*filename:* **color**

*format:* `idx,<f>,<t>,<values>`

f = color format: r - RGB, h - HSV<br>
t = s - solid, r - random, l - list

values:

- *solid:* color<br>example: "23,h,s,5,50,100"
- *random:* start color, end color<br>example: "24,r,r,64,64,64,240,200,128"
- *list:* number of colors, list of colors<br>example: "33,r,l,2,22,25,64,128,128,255"

### brightness
brightness of a pixel
- constant (val)
- blinking (period, ratio)
- flickr (min val, max val,speed)

*filename* **brightness**

*format:* `idx,<t>,<values>`

t = c - constant, b - blinking, f - flickr<br>

values:

- *constant:* value in %, 0..100<br>example: "52,c,100"
- *blinking:* period,ratio (period in ms, ratio in %)<br>example: "0,b,500,50"
- *flickr:* min val, max val<br>example: "3,f,100,170"

### pixel

it is a single pixel

has
- color
- brightness

*filename* **pixel**

### movement

how to move a range
- shift left / right
- move to position

### range

has a
- start position
- movement
- set of **pixel**

### set

has a set of
- range

### events

Events to start or stop a **set**
- automatically on power up,
- manual start/stop
- in a time range ( from ... to)
- randomly

### scene

a **scene**<br>
has a list of **events**<br>
which has a list of **set**<br>
which has a list of **ranges**<br>
which has a list of **movements**<br>
which has a list **pixels**<br>
which has a **color** and a **brightness**


## alternative concepts

### scenes

- static content (range, color, brightness)
- rainbow (range, brightness)
- Color gradient (range, from (color,brightness), to(color,brightness))
- color list
- effects (type(random, flickr, ... ),range, some parameter)

