import time
import rtb
import logging
logging.basicConfig(logging.DEBUG)
from uasyncio.core import get_event_loop,sleep


get_event_loop().call_later(5000, lambda: print("5k: Time is %s" % time.time()))
get_event_loop().call_later(15000, lambda: print("15k: Time is %s" % time.time()))

loop = get_event_loop()
loop.run_forever()
