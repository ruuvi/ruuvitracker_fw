# TODO: shamelessly steal all the good ideas from https://gist.github.com/nvbn/80c7b434ee21c99f013d
# Like passing the valid GPS coordinates via yielding from queue 
import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep

class GPS:
    uart_lld = None # Low-Level UART
    uart = None # This is the parser

    def __init__(self):
        pass

    def start(self):
        # We might call start/stop multiple times and in stop we do not release VBACKUP by default
        if not rtb.pwr.GPS_VBACKUP.status():
            rtb.pwr.GPS_VBACKUP.request()
        rtb.pwr.GPS_VCC.request()
        rtb.pwr.GPS_ANT.request()
        self.uart_lld = pyb.UART(rtb.GPS_UART_N, 115200, timeout=0, read_buf_len=256)
        self.uart = uartparser.UARTParser(self.uart_lld)
        
        # TODO: Add NMEA parsing callbacks here
        self.uart.add_line_callback('startswith', '$', self.print_line)
        
        # The parsers start method is a generator so it's called like this
        get_event_loop().create_task(self.uart.start())

    # TODO: Add GPS command methods (like setting the interval, putting the module to various sleep modes etc)
    
    def print_line(self, line, parser):
        print(line)
        parser.flush()
    
    def stop(self):
        self.uart.stop()
        self.uart_lld.deinit()
        rtb.pwr.GPS_VCC.release()
        rtb.pwr.GPS_ANT.release()
        # GPS_VBACKUP is left ureleased on purpose to allow for warm starts

instance = GPS()
