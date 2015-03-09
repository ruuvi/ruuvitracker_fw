import rtb
gps = pyb.UART(rtb.GPS_UART_N, 115200, timeout=0, read_buf_len=256)

if not rtb.pwr.GPS_VBACKUP.status():
    rtb.pwr.GPS_VBACKUP.request()

if not rtb.pwr.GPS_VCC.status():
    rtb.pwr.GPS_VCC.request()

if not rtb.pwr.GPS_ANT.status():
    rtb.pwr.GPS_ANT.request()

# uasyncio version
import logging
#logging.basicConfig(logging.DEBUG)
import uartparser
from uasyncio.core import get_event_loop, sleep

def print_line(line, parser):
    print("== LINE ===")
    print(line)
    print("===")
    parser.flush()


def print_match(match, parser):
    print ("=== MATCH ===")
    print(repr(match.group(0)))
    print("=== SUBGROUPS ===")
    for n in range(1,10):
        try:
            print("  %d: %s" % (n, match.group(n)))
        except IndexError:
            break
    print("===")
    #parser.flush()


p = uartparser.UARTParser(gps)
p.add_line_callback('startswith', '$GPRMC', print_line)
p.add_line_callback('startswith', '$GPVTG', print_line)
p.add_re_callback('GPGGA,(.*?),.*?,(.*?)\r\n', print_match)
loop = get_event_loop()
loop.call_soon(p.start())
loop.run_forever()


# Works
while True:
    if gps.any():
        received = gps.readchar()
        print(chr(received), end="")

# This hangs on REPL, maybe it waits for EOF??
#while True:
#    if gps.any():
#        print("Have data, read it all")
#        received = gps.readall()
#        print(received, end="")

# Sorta works, the print just prints the buffer funny
while True:
    if gps.any():
        received = gps.read(100) # Using 10 as the "at most" seems to result in single result almost always anyway
        print(received)
    else:
        print("Sleeping")
        pyb.delay(10)


