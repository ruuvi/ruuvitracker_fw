import pyb
import rtb

import logging
#logging.basicConfig(logging.DEBUG)
# rtb module patched the eventloop already
#import rtb.eventloop
from uasyncio.core import get_event_loop, sleep

def busylooper():
    while True:
        # TODO: Find out why even though we do yield we manage to block the sleeping tasks...
        yield

get_event_loop().create_task(rtb.heartbeat(1))
get_event_loop().create_task(busylooper())
get_event_loop().create_task(rtb.heartbeat(2))

get_event_loop().run_forever()
