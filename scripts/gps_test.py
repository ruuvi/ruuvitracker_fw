import rtb

import logging
#logging.basicConfig(logging.DEBUG)
from uasyncio.core import get_event_loop

from rtb.gps import instance as gps
rtb.pwr.GPS_VCC.status()

gps.start()

loop = get_event_loop()
loop.run_forever()

gps.stop()
rtb.pwr.GPS_VCC.status()

