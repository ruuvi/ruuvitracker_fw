# boot.py -- run on boot-up
# can run arbitrary Python, but best to keep it minimal

# This will help debugging a lot
import micropython
micropython.alloc_emergency_exception_buf(100)

# Import the board modules
import pyb
import rtb

# Note that main.py is always run if this is not specified...
#pyb.main('main.py') # main script to run after this one

# Left because included in default boot.py
##pyb.usb_mode('CDC+MSC') # act as a serial and a storage device
##pyb.usb_mode('CDC+HID') # act as a serial device and a mouse
