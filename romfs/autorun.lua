print("Starting RuuviTracker eLua FW\n")

print("Set green led ON\n")

led = pio.PD_12
pio.pin.setdir( pio.OUTPUT, led )
pio.pin.sethigh( led )

