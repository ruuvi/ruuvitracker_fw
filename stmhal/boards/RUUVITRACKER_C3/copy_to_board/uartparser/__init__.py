"""UART parser helper, requires uasyncio"""
import pyb
from uasyncio.core import get_event_loop, sleep

class UARTParser():
    recv_bytes = b''
    EOL = b'\r\n'
    line = b'' # Last detected complete line without EOL marker
    sleep_time = 0.01 # When we have no data sleep this long

    _run = False
    _sol = 0 # Start of line
    _line_cbs = [] # list of 3 value tuples (function, comparevalue, callback)
    _re_cbs = [] # list of 2 value tuples (re, callback)

    def __init__(self, uart):
        self.uart = uart

    def flush(self):
        self.recv_bytes = b''
        line = b''
        _sol = 0

    def parse_buffer(self):
        if (self.recv_bytes[-len(self.EOL):] == self.EOL):
            # End Of Line detected
            self.line = self.recv_bytes[self._sol:-len(self.EOL)]
            for cbinfo in self._line_cbs:
                if getattr(self.line, cbinfo[0])(cbinfo[1]):
                    get_event_loop().call_soon(cbinfo[2], self.line)
            # And finally point the start-of-line to current byte
            self._sol = len(self.recv_bytes)
        
        for cbinfo in self._re_cbs:
            #print("Checking %s with %s" % (repr(self.recv_bytes), repr(cbinfo[0])))
            match = cbinfo[0](self.recv_bytes)
            if match:
                 get_event_loop().call_soon(cbinfo[1], match)

    def add_re_callback(self, regex, cb, method='search'):
        """Adds a regex callback for checking the buffer every time we receive data (this obviously can get a bit expensive), takes the regex as string and callback function.
        Optionally you can specify 'match' instead of search as the method to use for matching. The callback will receive the match object."""
        import ure
        re = ure.compile(regex)
        self._re_cbs.append((getattr(re, method), cb))

    def add_line_callback(self, method, checkstr, cb):
        """Adds a callback for checking full lines, the method can be name of any valid bytearray method but 'startswith' and 'endswith' are probably the good choices.
        The check is performed (and callback will receive  the matched line) with End Of Line removed."""
        # Check that the method is valid
        getattr(self.recv_bytes, method)
        self._line_cbs.append((method, checkstr, cb))

    def start(self):
        self._run = True
        while self._run:
            # Apparently we can't do this...
            #recv = yield from self.uart.read(1)
            recv = self.uart.read(1)
            # Timed out
            if len(recv) == 0:
                yield from sleep(self.sleep_time)
                continue
            self.recv_bytes += recv
            get_event_loop().call_soon(self.parse_buffer)
            if not self.uart.any():
                # No data, sleep a bit
                yield from sleep(self.sleep_time)
            # Otherwise just yield (so the callbacks get processed)
            yield

    def stop(self):
        self._run = False

