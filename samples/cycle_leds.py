# Usage:
# import cycle_leds
# Interruct by control-C

import time
import leds

green = leds.Led('green')
red = leds.Led('red')
while True:
    green.on()
    red.off()
    time.sleep(1)
    green.off()
    red.on()
    time.sleep(1)
