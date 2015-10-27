# Handle the SD Card in Python until we have support for SPI cards in uPy core
import pyb, rtb
from . import sdcardlib
card_instance = None
# Active-low signal for SD detect
pyb.Pin.board.SD_CARD_INSERTED.init(pyb.Pin.IN, pyb.Pin.PULL_UP)

# Errors
class SDCardError(RuntimeError):
    pass

class NoCardError(SDCardError):
    pass



def present():
    # Active-low signal for SD detect
    return not pyb.Pin.board.SD_CARD_INSERTED.value()

def mount():
    global card_instance
    if card_instance:
        raise SDCardError("Already mounted")
    rtb.pwr.SDCARD.request()
    if pyb.Pin.board.SD_CARD_INSERTED.value():
        raise NoCardError("No card detected")
    card_instance = sdcardlib.SDCard(pyb.SPI(1), pyb.Pin.board.MICROSD_CS)
    pyb.mount(card_instance, '/sd')

def unmount():
    global card_instance
    pyb.mount(None, '/sd')
    card_instance = None
    rtb.pwr.SDCARD.release()

