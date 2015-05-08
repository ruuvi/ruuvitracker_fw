# TODO: shamelessly steal all the good ideas from https://gist.github.com/nvbn/80c7b434ee21c99f013d
# Like passing the valid GPS coordinates via yielding from queue 
import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep
import nmea
from nmea import FIX_TYPE_NONE, FIX_TYPE_2D, FIX_TYPE_3D
from nmea import MSG_GPRMC, MSG_GPGSA, MSG_GPGGA


# The handler class
class GPS:
    uart_wrapper = None # Low-Level UART
    uart = None # This is the parser
    last_fix = None
    _next_fix = None

    def __init__(self):
        pass

    def start(self):
        self.uart_wrapper = uartparser.UART_with_fileno(rtb.GPS_UART_N, 115200, read_buf_len=256)
        self.uart = uartparser.UARTParser(self.uart_wrapper)


        # TODO: Add NMEA parsing callbacks here
        self.uart.add_re_callback(r'GGA', r'^\$G[PLN]GGA,.*', self.gpgga_received)
        self.uart.add_re_callback(r'GSA', r'^\$G[PLN]GSA,.*', self.gpgsa_received)
        self.uart.add_re_callback(r'RMC', r'^\$G[PLN]RMC,.*', self.gprmc_received)
        
        # Start the parser
        get_event_loop().create_task(self.uart.start())

        # Assert wakeup
        rtb.GPS_WAKEUP_PIN.high()

        # And turn on the power
        # We might call start/stop multiple times and in stop we do not release VBACKUP by default
        if not rtb.pwr.GPS_VBACKUP.status():
            rtb.pwr.GPS_VBACKUP.request()
        rtb.pwr.GPS_ANT.request()
        rtb.pwr.GPS_VCC.request()

        # Just to keep consistent API, make this a coroutine too
        yield

    # TODO: Add GPS command methods (like setting the interval, putting the module to various sleep modes etc)
    # @see https://github.com/RuuviTracker/ruuvitracker_hw/blob/revC3/datasheets/SIM28_SIM68R_SIM68V_NMEA_Messages_Specification_V1.01.pdf

    def gprmc_received(self, match):
        line = match.group(0)
        # Skip checksum failures
        if not nmea.checksum(line):
            return

        if not self._next_fix:
            self._next_fix = nmea.Fix()
        nmea.parse_gprmc(line, self._next_fix)
        if self._next_fix.lat != None:
            self.last_fix = self._next_fix
            self.last_fix.last_update = pyb.millis()
            self._next_fix = None
            print("===\r\nRMC lat=%s lon=%s altitude=%s\r\n==" % (self.last_fix.lat, self.last_fix.lon, self.last_fix.altitude))
            # TODO: Check if anyone wants to see the fix yet

    def gpgga_received(self, match):
        line = match.group(0)
        # Skip checksum failures
        if not nmea.checksum(line):
            return

        if not self._next_fix:
            self._next_fix = nmea.Fix()
        nmea.parse_gpgga(line, self._next_fix)
        if self._next_fix.lat != None:
            print("===\r\nGGA lat=%s lon=%s altitude=%s\r\n==" % (self._next_fix.lat, self._next_fix.lon, self._next_fix.altitude))

    def gpgsa_received(self, match):
        line = match.group(0)
        # Skip checksum failures
        if not nmea.checksum(line):
            return

        if not self._next_fix:
            self._next_fix = nmea.Fix()
        nmea.parse_gpgsa(line, self._next_fix)
        if self._next_fix.lat != None:
            print("===\r\nGSA lat=%s lon=%s altitude=%s\r\n==" % (self._next_fix.lat, self._next_fix.lon, self._next_fix.altitude))

    def set_interval(self, ms):
        """Set update interval in milliseconds"""
        resp = yield from self.uart.cmd(nmea.checksum("$PMTK300,%d,0,0,0,0" % ms))
        print("set_interval: Got response: %s" % resp)
        # TODO: Check the response somehow ?

    def set_standby(self, state):
        """Set or exit the standby mode, set to True or False"""
        resp = yield from self.uart.cmd(nmea.checksum("$PMTK161,%d" % state))
        print("set_standby: Got response: %s" % resp)
        # TODO: Check the response somehow ?

    def stop(self):
        self.uart.del_re_callback('RMC')
        self.uart.del_re_callback('GGA')
        self.uart.del_re_callback('GSA')
        self.uart.del_line_callback('all')
        # Drive the wakeup pin low
        rtb.GPS_WAKEUP_PIN.low()
        yield from self.uart.stop()
        self.uart_wrapper.deinit()
        rtb.pwr.GPS_VCC.release()
        rtb.pwr.GPS_ANT.release()
        # GPS_VBACKUP is left ureleased on purpose to allow for warm starts


instance = GPS()
