# Usage:
# l = leds.Led('green')
# l.on()
# l.off()

import pyb

class Led:
  pin = None
  light = False

  def __init__(self, color):
    if (color == 'green'):
      self.pin = pyb.Pin('LED_GREEN', pyb.Pin.OUT_PP)
    elif (color == 'red'):
      self.pin = pyb.Pin('LED_RED', pyb.Pin.OUT_PP)
    else:
      print('Unknown led color')

  def on(self):
    if self.pin:
      self.pin.high()
      self.light = True

  def off(self):
    if self.pin:
      self.pin.low()
      self.light = False

  def toggle(self):
    if self.pin:
      if self.light:
        self.off()
      else:
        self.on()

  def is_lit(self):
    return self.light
