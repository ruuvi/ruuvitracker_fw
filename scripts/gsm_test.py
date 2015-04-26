import rtb
import logging
logging.basicConfig(logging.DEBUG)
from uasyncio.core import get_event_loop,sleep
from rtb.gsm import instance as gsm
rtb.pwr.GSM_VBAT.status()

get_event_loop().create_task(gsm.start())
#get_event_loop().create_task(rtb.heartbeat(1))
get_event_loop().call_later(10000, gsm.at_test())

loop = get_event_loop()
loop.run_forever()

get_event_loop().run_until_complete(gsm.stop())
rtb.pwr.GSM_VBAT.status()


#get_event_loop().run_until_complete(gsm.start())
#get_event_loop().run_until_complete(gsm.at_test())

