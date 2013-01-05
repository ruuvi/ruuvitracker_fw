require('common')
require('led')
require('tracker')
require('gsm')
require('gps')

--Start booting
red_led:on()
term.clrscr()
print("Starting RuuviTracker eLua FW", firmware.version)
print("Board: " .. pd.board() )

gsm.setup()
gsm.start_gprs()
gps.enable()

print("Memory usage: ", collectgarbage('count'), "kB")
print("All devices enabled")

--boot sequence done, notify user
red_led:off()

tracker.send_event( {annotation = "boot" } ) -- Send first event, to notify server we are alive

print("Starting GPS loop. Press 'q' to stop")	
while uart.read(0,1,0)~='q' do
   gps.process()
   if gps.is_fixed then
      green_led:on()
      tracker.send_event( gps.event )
   else
      green_led:off()
   end
end