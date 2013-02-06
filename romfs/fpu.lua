-- Floating point speed test

print("Starting Floating point speed test")

local function test(n)
   t1 = tmr.start(0)
   local x=0
   for i=1,n do
      x= 1e3 * 1e3
   end
   t2 = tmr.read(0)
   print(n, "loops time=", tmr.gettimediff(0, t1, t2 ), "us")
end

test(10)
test(100)
test(1e3)
test(1e6)
