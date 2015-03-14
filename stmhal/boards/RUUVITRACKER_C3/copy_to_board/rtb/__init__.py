import pyb
from .powerdomains import powermanager_singleton
from . import powerdomains_config as pwr

pwrmgr = powermanager_singleton

# 2015-03-08 rambo: I *think* these are correct UART numbers, GPS at least is.
GSM_UART_N = 3 # PB10/PB11 and PB13/PB14 (flow control), see http://forum.micropython.org/viewtopic.php?t=376
GPS_UART_N = 2 # PA2/PA3
# We probably need to remap this with some alternate-function system or something...
#GSM_DGB_UART = 1 # PA9/PA10
GSM_DGB_UART_N = None
# From micropython/stmhal/uart.c
#///   - `UART(4)` is on `XA`: `(TX, RX) = (X1, X2) = (PA0, PA1)`
#///   - `UART(1)` is on `XB`: `(TX, RX) = (X9, X10) = (PB6, PB7)`
#///   - `UART(6)` is on `YA`: `(TX, RX) = (Y1, Y2) = (PC6, PC7)`
#///   - `UART(3)` is on `YB`: `(TX, RX) = (Y9, Y10) = (PB10, PB11)`
#///   - `UART(2)` is on: `(TX, RX) = (X3, X4) = (PA2, PA3)`
