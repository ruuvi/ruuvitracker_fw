import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep

class GSM:
    uart_wrapper = None # Low-Level UART
    uart = None # This is the parser

    def __init__(self):
        pass

    def start(self):
        # Try with flow control
        self.uart_wrapper = uartparser.UART_with_fileno(rtb.GSM_UART_N, 115200, read_buf_len=256, flow=pyb.UART.RTS | pyb.UART.CTS)
        # TODO: schedule something that will reset the board to autobauding mode if it had not initialized within X seconds
        self.uart = uartparser.UARTParser(self.uart_wrapper)

        # TODO: URC parsers here
        self.uart.add_line_callback('pls', 'startswith', '+', self.print_line)

        # The parsers start method is a generator so it's called like this
        get_event_loop().create_task(self.uart.start())

        # TODO: Doublecheck the schema, was there a to control line for GSM RTC as well ??
        rtb.pwr.GSM_VBAT.request()
        
        get_event_loop().create_task(self.push_powerbutton())


        # Just to keep consistent API, make this a coroutine too
        yield
        
    def push_powerbutton(self, push_time=2000):
        rtb.GSM_PWR_PIN.low()
        yield from sleep(push_time)
        rtb.GSM_PWR_PIN.high()
        

    # TODO: The GPIO song&dance with the GSM "powerbutton"
    
    # TODO: Autobauding -> set baud and flow control (rmember to reinit the UART...)

    # TODO: Add GSM command methods (like setting the interval, putting the module to various sleep modes etc)
    
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
        self.uart.stop()
        self.uart_wrapper.deinit()
        rtb.pwr.GSM_VBAT.release()

instance = GSM()
