import rtb

gps = pyb.UART(rtb.GPS_UART_N, 115200)

if not rtb.pwr.GPS_VBACKUP.status():
    rtb.pwr.GPS_VBACKUP.request()
if not rtb.pwr.GPS_VCC.status():
    rtb.pwr.GPS_VCC.request()
if not rtb.pwr.GPS_ANT.status():
    rtb.pwr.GPS_ANT.request()

while True:
    if gps.any():
        received = gps.readchar()
        print(chr(received), end="")
