require('config')
require('led')
require('tracker')
require('gsm')
require('gps')
require('logging')
require('http')

-- Start booting
red_led:on()
term.clrscr()
print("Starting RuuviTracker eLua FW", firmware.version)
print("Board: " .. pd.board())

local ok
local msg

local run_test = false

function test_func()
   while true do
      gsm.wait_ready()
      while run_test == false do
	 coroutine.yield()
      end
      print("Test func started")
      print(http.get("http://www.google.fi/"))
      run_test = false
   end
end

-- List of handlers to process on main loop
local handlers = {
   gsm.handler,
   gps.handler,
   tracker.handler
}

function print_mem()
   print("Memory usage: ", collectgarbage('count'), "kB")
end

Logger.config['gsm'] = Logger.level.DEBUG
Logger.config['gps'] = Logger.level.DEBUG

local mem_timeout = 10e6 -- 10s
tmr.setclock(firmware.timers.mem_timer, 2e3 ) -- 2kHz is known to work (on ruuviA)
local counter = tmr.start(firmware.timers.mem_timer)

print("Starting main loop...")

local function mainloop()
   -- Run through all handler (coroutines)
   for _, handler in ipairs(handlers) do
      if coroutine.status(handler) == 'suspended' then
	 ok, msg = coroutine.resume(handler)
	 if not ok then print("Error:", msg) end
      end
   end

   -- Other functions, outside of handlers
   if gps.is_fixed then green_led:on()
   else green_led:off() end
   if gsm.is_ready() then
      red_led:off()
   else
      red_led:on()
   end
   
   local delta = tmr.gettimediff(firmware.timers.mem_timer, counter, tmr.read(firmware.timers.mem_timer))
   if delta > mem_timeout then
      print_mem()
      counter = tmr.start(firmware.timers.mem_timer) -- Reset timer
   end

   -- Keypress handlers
   local key = uart.read(0,1,0)
   if key == '1' then
      gsm.idle_allowed = true
   elseif key == '2' then
      gsm.idle_allowed = false
   elseif key == '3' then
      gsm.sleep_allowed = true
   elseif key == '4' then
      gsm.sleep_allowed = false
   elseif key == 'g' then
      if gps.sleep_allowed then
	 gps.sleep_allowed = false
      else
	 gps.sleep_allowed = true
      end
   elseif key == 't' then
      run_test = true
   end
end

ruuvi.idleloop(mainloop) -- Run function in loop. Never returns
