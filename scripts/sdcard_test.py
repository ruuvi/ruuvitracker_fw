# Using SDcard in RuuviTracker
import os, pyb, rtb
import rtb.sdcard
print(rtb.sdcard.present())
rtb.sdcard.mount()
os.listdir('/')
os.listdir('/spisd')
open('/spisd/hello.txt','r').read()
rtb.sdcard.unmount()
os.listdir('/')

