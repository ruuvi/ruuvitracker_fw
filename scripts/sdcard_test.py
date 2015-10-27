# Using SDcard in RuuviTracker
import os, pyb, rtb
import rtb.sdcard
print(rtb.sdcard.present())
rtb.sdcard.mount()
os.listdir('/sd')
open('/sd/hello.txt','r').read()
