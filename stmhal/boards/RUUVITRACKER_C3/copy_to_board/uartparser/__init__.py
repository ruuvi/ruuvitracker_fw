"""UART parser helper, requires uasyncio"""
import pyb
import rtb.eventloop
from uasyncio import get_event_loop, sleep, StreamReader, StreamWriter

# Errors
class UARTParserError(RuntimeError):
    pass

class CommandTimeout(UARTParserError):
    pass

class CallbackError(UARTParserError):
    pass



class UART_with_fileno:
    """Wraps the pyb.UART object (we cannot just subclass from it, select.poll will choke) to have support for fileno() to be compatible with uasyncio IORead et co"""
    uart_lld = None

    def __init__(self, *args, **kwargs):
        self.uart_lld = pyb.UART(*args, **kwargs)

    def fileno(self):
        """The IO Scheduling eventloops expect io objects to have fileno() method, MicroPythons select.poll expects the UART/USB object"""
        return self.uart_lld

    def __getattr__(self, attr):
        return getattr(self.uart_lld, attr)



class RWStream(StreamReader, StreamWriter):
    pass



class UARTParser():
    recv_bytes = b''
    EOL = b'\r\n'
    line = b'' # Last detected complete line without EOL marker
    uart = None
    stream = None

    _run = False
    _stopped = True
    _sol = 0 # Start of line
    _str_cbs = {} # map of 3 value tuples (functionname, comparevalue, callback)
    _re_cbs = {} # map of 2 value tuples (compiled_re, callback)
    _raw_cb = None
    _cmd_cb = None
    _cmd_line = None

    def __init__(self, uart):
        self.uart = uart
        self.stream = RWStream(self.uart)

    def flush(self):
        self.recv_bytes = b''
        self.line = b''
        self._sol = 0

    def flushto(self, pos):
        self.recv_bytes = self.recv_bytes[pos:]
        if pos > self._sol:
            self.line = b''
            self._sol = 0

    def enter_raw(self, cb):
        """Enters "raw mode" where the given callback is called (with reference to the parser as argument) whenever there is new data, all other parsing is suspended, remember to flush the buffers manually too!"""
        self._raw_cb = cb

    def exit_raw(self, flush=True):
        """Exits raw mode, automatically flushes the buffer unless told not to"""
        self.flush()
        self._raw_cb = None

    def cmd(self, cmd, timeout=1000):
        """Send a command string to the uart and returns next line as response, call with value = yield from parser.cmd("AT"), linebreak is added automatically"""
        # If parser was not running start it until we are done (the eventloop obviously must be running)
        stop_when_done = False
        if not self._run:
            # Start the parser coroutine
            get_event_loop().create_task(self.start())
            stop_when_done = True
        self._cmd_line = None
        # This is the callback for the parser
        def _cb(recv):
            self._cmd_line = recv
            self._cmd_cb = None
        self._cmd_cb = _cb

        # PONDER: Wait for the receive buffer to be empty ??
        # This claims to return immediately
        yield from self.stream.awrite(b'%s%s' % (cmd, self.EOL.decode('ascii')))
        # This might block but awrite will also first call the write and only then if it was a partial write schedule next one...
        #self.uart.write(b'%s%s' % (cmd, self.EOL.decode('ascii')))
        # Loop until timeout or received line
        started = pyb.millis()
        while not self._cmd_line:
            yield from sleep(10)
            if pyb.elapsed_millis(started) > timeout:
                self._cmd_line = CommandTimeout()
        if stop_when_done:
            get_event_loop().create_task(self.stop())
        return self._cmd_line

    def parse_buffer(self):
        if self._raw_cb:
            # PONDER: Should we raise an exception ?
            return

        eolpos = self.recv_bytes.find(self.EOL, self._sol)
        while eolpos > -1:
            # End Of Line detected
            self.line = self.recv_bytes[self._sol:eolpos]
            flushnow = True
            print("_line=%s" % self.line)
            
            # The special case of a command callback
            if self._cmd_cb:
                if self._cmd_cb(self.line):
                    flushnow = False
                if flushnow:
                    self.flushto(eolpos+len(self.EOL))
                else:
                    # Point the start-of-line to next line
                    self._sol = eolpos+len(self.EOL)
                continue

            for cbid in self._str_cbs:
                cbinfo =  self._str_cbs[cbid]
                if getattr(self.line, cbinfo[0])(cbinfo[1]):
                    if (cbinfo[2](self.line)):
                        flushnow = False

            for cbid in self._re_cbs:
                cbinfo =  self._re_cbs[cbid]
                match = cbinfo[0](self.line)
                if match:
                    if (cbinfo[1](match)):
                        flushnow = False

            if flushnow:
                self.flushto(eolpos+len(self.EOL))
            else:
                # Point the start-of-line to next line
                self._sol = eolpos+len(self.EOL)
            # And loop, just in case we have multiple lines in the buffer...
            eolpos = self.recv_bytes.find(self.EOL, self._sol)

    def add_re_callback(self, cbid, regex, cb, method='search'):
        """Adds a regex callback for checking full lines, takes the regex as string and callback function. Optionally you can specify 'match' instead of 'search' as the method to use for matching.
        The callback will receive the match object. Return True from the callback to prevent flushing the buffer. NOTE: End Of Line is not part of the line, omit that from your regex too"""
        import ure
        # Sanity-check
        if cbid in self._re_cbs:
            raise CallbackError("Trying to add same callback twice")
        # Compile the regex
        re = ure.compile(regex)
        # And add the the callback list
        self._re_cbs[cbid] = (getattr(re, method), cb)

    def del_re_callback(self, cbid):
        """Removes a regex callback"""
        if cbid in self._re_cbs:
            del(self._re_cbs[cbid])
            return True
        return False

    def add_line_callback(self, cbid, method, checkstr, cb):
        """Adds a callback for checking full lines, the method can be name of any valid bytearray method but 'startswith' and 'endswith' are probably the good choices.
        The check is performed (and callback will receive  the matched line) with End Of Line removed. Return True from the callback to flush the buffer"""
        # Sanity-check
        if cbid in self._str_cbs:
            raise CallbackError("Trying to add same callback twice")
        # Check that the method is valid
        getattr(self.recv_bytes, method)
        # And add the the callback list
        self._str_cbs[cbid] = (method, checkstr, cb)

    def del_line_callback(self, cbid):
        """Removes a line callback"""
        if cbid in self._str_cbs:
            del(self._str_cbs[cbid])
            return True
        return False

    def start(self):
        if self._run:
            # Just in case someone calls this twice...
            return
        self._run = True
        self._stopped = False
        while self._run:
            recv = yield from self.stream.read(100)
            # TODO: Check if we get an error
            self.recv_bytes += recv
            if not self._raw_cb:
                # TODO: We may want to inline the parsing due to cost of method calls
                self.parse_buffer()
            else:
                self._raw_cb(self)
        self._stopped = True

    def stop(self):
        self._run = False
        # Wait for the start-coroutine to complete stopping itself
# Actually this might not be such a good idea afterall
#        while not self._stopped:
#            yield from sleep(10)
        # Keep it a coroutine though
        yield

