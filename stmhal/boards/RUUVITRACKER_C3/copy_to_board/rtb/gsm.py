import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep


# TODO: This thing needs a proper state machine to keep track of the sleep modes

class GSM:
    uart_wrapper = None # Low-Level UART
    uart = None # This is the parser

    def __init__(self):
        pass

    def start(self):
        # Try without flow control first
        self.uart_wrapper = uartparser.UART_with_fileno(rtb.GSM_UART_N, 115200, read_buf_len=256)
        #self.uart_wrapper = uartparser.UART_with_fileno(rtb.GSM_UART_N, 115200, read_buf_len=256, flow=pyb.UART.RTS | pyb.UART.CTS)
        # TODO: schedule something that will reset the board to autobauding mode if it had not initialized within X seconds
        self.uart = uartparser.UARTParser(self.uart_wrapper)

        # TODO: URC parsers here
        self.uart.add_line_callback('pls', 'startswith', '+', self.print_line)

        # The parsers start method is a generator so it's called like this
        get_event_loop().create_task(self.uart.start())

        # TODO: Doublecheck the schema, was there a to control line for GSM RTC as well ??
        rtb.pwr.GSM_VBAT.request()
        
        yield from self.push_powerbutton()
        # Assert DTR to enable module UART (we can also sample the DTR pin to see if the module is powered on
        rtb.GSM_DTR_PIN.low()

        # Just to keep consistent API, make this a coroutine too
        yield
        
    def push_powerbutton(self, push_time=2000):
        rtb.GSM_PWR_PIN.low()
        yield from sleep(push_time)
        rtb.GSM_PWR_PIN.high()

    # TODO: Autobauding -> set baud and flow control (rmember to reinit the UART...)

    # TODO: Add GSM command methods (putting the module to various sleep modes etc)
    
    # TODO: Add possibility to attach callbacks to SMS received etc

    def at_test(self):
        print("at_test called")
        resp = yield from self.uart.cmd("AT")
        print("at_test: Got response: %s" % resp)

    def print_line(self, line, parser):
        print(line)
        return True
    
    def stop(self):
        self.uart.del_line_callback('pls')
        yield from self.push_powerbutton()
        yield from self.uart.stop()
        self.uart_wrapper.deinit()
        # de-assert DTR
        rtb.GSM_DTR_PIN.high()
        rtb.pwr.GSM_VBAT.release()

instance = GSM()
