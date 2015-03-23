import pyb
import uasyncio.core

class RTEventLoop(uasyncio.core.EventLoop):
    def time(self):
        return pyb.millis()

    def wait(self, delay):
        start = pyb.millis()
        while pyb.elapsed_millis(start) < delay:
            pyb.delay(1)

uasyncio.core._event_loop = None
uasyncio.core._event_loop_class = RTEventLoop
