# Usage:
# import cycle_leds

import time
import leds

green = leds.Led('green')
red = leds.Led('red')
green.on()
red.off()
while True:
    green.toggle()
    red.toggle()
    time.sleep(1)
