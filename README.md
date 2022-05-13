# esp32_ws2812
control a ws2812-lightstrip with an wsp32

the plan to use a dastLED-Library was not successful because of timing troubles when integrating a web server.

I've found some ws2812 code in the esp-idf examples. This works parallel with the http server.


## Concepts

### color
color of a pixel, formats are RGB or HSV
- solid
- random
- as list of colors

### brightness
brightness of a pixel
- constant (val)
- blinking (on-time, off-time)
- flickr (min val, max val)

### pixel

it is a single pixel

has
- color
- brightness

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