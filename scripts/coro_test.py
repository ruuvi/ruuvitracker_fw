import pyb
import rtb

import logging
#logging.basicConfig(logging.DEBUG)
from uasyncio.core import get_event_loop, sleep

# This is a coroutine, we do not use the decorator to indicate that due to resource constrainst of pyboard
def heartbeat(ledno=1):
    led = pyb.LED(ledno)
    sleep1 = 2
    sleep2 = 0.1
    sleep3 = 0.2
    while True:
        # TODO: find out why this also prevents the other hearbeat task from running (leading to effective 4s delay for single task [and worse, all other tasks are fscked too ??])
        yield from sleep(sleep1)
        led.on()
        yield from sleep(sleep2)
        led.off()
        yield from sleep(sleep3)
        led.on()
        yield from sleep(sleep2)
        led.off()


get_event_loop().create_task(heartbeat(1))
get_event_loop().create_task(heartbeat(2))

get_event_loop().run_forever()
