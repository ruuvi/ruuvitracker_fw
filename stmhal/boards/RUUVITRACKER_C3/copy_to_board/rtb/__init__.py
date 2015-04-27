import pyb
# Muck the uasyncio default eventloop as soon as rtb is imported
from . import eventloop
from uasyncio.core import sleep

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

# Default the open-drains to high, this is run only on the first module import so it does not cause weird side-effect when someone imports rtb to access features
# NOTE: Even though these have external pull-ups, they're only enabled when the GSM module is powered on, this is good to remember if you want to sample the external value
GSM_PWR_PIN = pyb.Pin('GSM_PWRKEY')
GSM_PWR_PIN.high()
GSM_PWR_PIN.init(pyb.Pin.OUT_OD)

GSM_DTR_PIN = pyb.Pin('GSM_DTR')
GSM_DTR_PIN.high()
GSM_DTR_PIN.init(pyb.Pin.OUT_OD)

GPS_WAKEUP_PIN = pyb.Pin('GPS_WAKEUP', pyb.Pin.OUT_PP)


# This is a coroutine, we do not use the decorator to indicate that due to resource constrainst of pyboard
def heartbeat(ledno=1):
    led = pyb.LED(ledno)
    sleep1 = 2000
    # It seems that when RTC is working we cannot really do subsecond sleeps, and if it's not working we cannot do concurrent sleeps...
    sleep2 = 100
    sleep3 = 250
    while True:
        yield from sleep(sleep1)
        led.on()
        yield from sleep(sleep2)
        led.off()
        yield from sleep(sleep3)
        led.on()
        yield from sleep(sleep2)
        led.off()
