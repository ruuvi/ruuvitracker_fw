import pyb
import rtb
import uartparser
from uasyncio.core import get_event_loop, sleep

class GSM:
    uart_lld = None # Low-Level UART
    uart = None # This is the parser

    def __init__(self):
        pass

    def start(self):
        # TODO: Doublecheck the schema, was there a to control line for GSM RTC as well ??
        rtb.pwr.GSM_VBAT.request()
        self.uart_lld = pyb.UART(rtb.GSM_UART_N, 115200, timeout=0, read_buf_len=256)
        self.uart = uartparser.UARTParser(self.uart_lld)
        
        # TODO: URC parsers here
        self.uart.add_line_callback('pls', 'startswith', '+', self.print_line)
        
        # The parsers start method is a generator so it's called like this
        get_event_loop().create_task(self.uart.start())

    # TODO: The GPIO song&dance with the GSM "powerbutton"
    
    # TODO: Autobauding -> set baud and flow control (rmember to reinit the UART...)

    # TODO: Add GSM command methods (like setting the interval, putting the module to various sleep modes etc)
    
    # TODO: Add possibility to attach callbacks to SMS received etc

    def print_line(self, line, parser):
        print(line)
        return True
    
    def stop(self):
        self.uart.del_line_callback('pls')
        self.uart.stop()
        self.uart_lld.deinit()
        rtb.pwr.GSM_VBAT.release()

instance = GSM()
