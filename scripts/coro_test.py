import pyb

import logging
#logging.basicConfig(logging.DEBUG)
import rtb.eventloop
from uasyncio.core import get_event_loop, sleep

# This is a coroutine, we do not use the decorator to indicate that due to resource constrainst of pyboard
def heartbeat(ledno=1):
    led = pyb.LED(ledno)
    sleep1 = 2000
    # It seems that when RTC is working we cannot really do subsecond sleeps, and if it's not working we cannot do concurrent sleeps...
    sleep2 = 100
    sleep3 = 250
    while True:
        yield from sleep(sleep1)
        led.on()
        #print("led %s ON" % ledno)
        yield from sleep(sleep2)
        led.off()
        #print("led %s OFF" % ledno)
        yield from sleep(sleep3)
        led.on()
        #print("led %s ON" % ledno)
        yield from sleep(sleep2)
        led.off()
        #print("led %s OFF" % ledno)
        #print("Looping back")

def busylooper():
    while True:
        # TODO: Find out why even though we do yield we manage to block the sleeping tasks...
        yield

get_event_loop().create_task(heartbeat(1))
get_event_loop().create_task(busylooper())
get_event_loop().create_task(heartbeat(2))

get_event_loop().run_forever()
