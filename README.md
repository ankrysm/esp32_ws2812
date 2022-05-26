# esp32_ws2812
control a ws2812-lightstrip with an wsp32

the plan to use a dastLED-Library was not successful because of timing troubles when integrating a web server.

I've found some ws2812 code in the esp-idf examples. This works parallel with the http server.


## Concepts

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

