import rtb
import logging
#logging.basicConfig(logging.DEBUG)
from uasyncio.core import get_event_loop,sleep
from rtb.gps import instance as gps
rtb.pwr.GPS_VCC.status()

get_event_loop().create_task(gps.start())
get_event_loop().create_task(rtb.heartbeat(1))
# this call_later thing doesn't quite seem to work event this way (the problem is that uart get data first and poll return "early"
get_event_loop().call_later(5000, lambda: get_event_loop().call_soon(gps.set_interval(5000)))
get_event_loop().call_later(10000, lambda: get_event_loop().call_soon(gps.set_interval(1000)))

loop = get_event_loop()
loop.run_forever()

get_event_loop().run_until_complete(gps.stop())
rtb.pwr.GPS_VCC.status()


import nmea
rmc = b'$GNRMC,193202.000,A,6007.2666,N,02423.8747,E,0.16,354.15,140315,,,A*76'
f = nmea.parse_gprmc(rmc.format()) # use .format to cast to string
f.received_messages & nmea.MSG_GPRMC

# Valid fix
gsa = b'$GNGSA,A,3,20,06,10,31,02,,,,,,,,1.79,1.54,0.92*1D'
# no fix
#gsa = b'$GNGSA,A,1,,,,,,,,,,,,,,,*00'
f = nmea.parse_gpgsa(gsa.format()) # use .format to cast to string
f.received_messages & nmea.MSG_GPGSA

# Valid fix
gga = b'$GPGGA,193202.000,6007.2666,N,02423.8747,E,1,8,1.97,19.4,M,19.8,M,,*62'
# No fix
#gga = b'$GPGGA,213053.790,,,,,0,0,,,M,,M,,*40'
f = nmea.parse_gpgga(gga.format()) # use .format to cast to string
f.received_messages & nmea.MSG_GPGGA

