from pyb import I2C

class mma8652:
    bus = None
    addr = None

    def __init__(self, addr=0x1d):
        self.bus = I2C(1, I2C.MASTER)
        self.addr = addr
        
    # TODO: Implement basic functions like actually getting acceleration data.
    
    # Implement configuration of interrupt sources etc
    
    # Implement calibration routines

    def interrupt_polarity(self, high=True, pushpull=True):
        """This configures the interrupt polarity, for the onboard it must be active-high and push-pull"""
        config = ord(self.bus.mem_read(1, self.addr, 0x2c, timeout=200))
        if high:
            config |= 0x2
        else:
            config &= 0xfd # (0xff ^ 0x2)
        if pushpull:
            config &= 0xfe # (0xff ^ 0x1)
        else:
            config |= 0x1
        self.bus.mem_write(chr(config), self.addr, 0x2c, timeout=200)
        
        #make this a coroutine
        yield
            

onboard = mma8652()
# Set interrupt output active-high push-pull
onboard.interrupt_polarity(True, True)
