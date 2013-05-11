-- Test code for LSM303DLHC
-- Linear acceleration address is 0011001b = 0x19
-- Magnetic interface address is 11110b (0x1E)

id = 0
addr = 0x19
reg = 0x28
REG1_A = 0x20
autoinc_bit = 7

-- Enable sensor axels X,Y,Z
-- REG_A: |ODR3|ODR2|ODR1|ODR0|LPen|Zen|Yen|Xen|
-- Send REGA:ORD(3:0) = 0100b Normal / low-power mode (50 Hz)
-- Enable bits 0,1,2,6
i2c.write_to(id, addr, REG1_A, bit.set(0,0,1,2,6))
i2c.write_to(id, addr, 0x23, bit.set(0,7)) -- set Reg_4_A:BDU=1

term.clrscr()
print("----------[ Linear accelerometer test | Pres 'Q' to stop ]----------------\n[ X ]\n[ Y ]\n[ Z ]\n")
local line = 2
local col  = 10
while uart.read(0,1,0) ~= 'q' do
   local bytes = i2c.read_from(0, addr, bit.set(reg,autoinc_bit), 6) -- Read all axes (3*2bytes)
   local _,x,y,z = pack.unpack(bytes,'<h3')
   -- Range +-2G , 16bit value. scale to 0..2
   x = x / 2^14
   y = y / 2^14
   z = z / 2^14
   term.print(col, line+0, tostring(x)) term.clreol()
   term.print(col, line+1, tostring(y)) term.clreol()
   term.print(col, line+2, tostring(z)) term.clreol()
   term.moveto(1,1)

   tmr.delay(0,5e4)
end
