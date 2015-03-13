# TODO: shamelessly steal all the good ideas from https://gist.github.com/nvbn/80c7b434ee21c99f013d
# Like passing the valid GPS coordinates via yielding from queue 
import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep
import nmea

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
        #self.uart.add_line_callback('all', 'startswith', '$', self.print_line)

        self.uart.add_re_callback('RMC', '\\$G[PLN]RMC,.*\r\n', self.gprmc_received)
        self.uart.add_re_callback('RMC', '\\$G[PLN]GGA,.*\r\n', self.gpgga_received)
        self.uart.add_re_callback('RMC', '\\$G[PLN]GSA,.*\r\n', self.gpgsa_received)
        
        # The parsers start method is a generator so it's called like this
        get_event_loop().create_task(self.uart.start())

    # TODO: Add GPS command methods (like setting the interval, putting the module to various sleep modes etc)

    def gprmc_received(self, match, parser):
        line = match.group(0)[:-3]
        print("$G[PLN]RMC=%s" %line)
        return True

    def gpgga_received(self, match, parser):
        line = match.group(0)[:-3]
        print("$G[PLN]GGA=%s" %line)
        return True

    def gpgsa_received(self, match, parser):
        line = match.group(0)[:-3]
        print("$G[PLN]GSA=%s" %line)
        return True

    def set_interval(self, ms):
        """Set update interval in milliseconds"""
        self.uart_lld.write(nmea.checksum("$PMTK300,%d,0,0,0,0\r\n" % ms))
        # TODO: Check the response somehow ?

    def set_standby(self, state):
        """Set or exit the standby mode, set to True or False"""
        self.uart_lld.write(nmea.checksum("$PMTK161,%d\r\n" % ms))
        # TODO: Check the response somehow ?

    def print_line(self, line, parser):
        print(line)
        return True
    
    def stop(self):
        self.uart.del_line_callback('all')
        self.uart.stop()
        self.uart_lld.deinit()
        rtb.pwr.GPS_VCC.release()
        rtb.pwr.GPS_ANT.release()
        # GPS_VBACKUP is left ureleased on purpose to allow for warm starts

instance = GPS()
