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


import nmea
s = b'$GPRMC,235954.800,V,,,,,0.00,0.00,050180,,,N*45'
nmea.checksum(s)
nmea.checksum(s[:-3])

