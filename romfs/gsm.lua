require('common')  -- load config

module('gsm', package.seeall)  -- Define module 'gsm'

power_pin = pio.PC_4

verbose = 0

local UARTID=2

-- Toggle power pin.
function poweron()
   print("GSM Power on..")
   pio.pin.setdir(pio.OUTPUT, power_pin)
   pio.pin.setlow(power_pin) 
   tmr.delay( 0, 1000000 )
   pio.pin.sethigh(power_pin)
   tmr.delay( 0, 500000 )
   uart.setup( UARTID, 9600, 8, uart.PAR_NONE, uart.STOP_1 )
   uart.set_flow_control( UARTID, uart.FLOW_RTS + uart.FLOW_CTS )
   uart.write(UARTID,"AT\r")
   tmr.delay( 0, 50000 )
end

-- Send string to GSM UART
-- append extra "\r" to serial port
function send(str)
   if verbose == 1 then print(str) end
   uart.write(UARTID, str.."\r")
end

-- Raw read bytes from GSM
function read_raw(nbytes)
   local len = tonumber(nbytes)
   local data = ""
   repeat
      local buf = uart.read(UARTID,len)
      len = len-buf:len()
      data = data .. buf
   until len == 0
   return data
end


-- Wait for string pattern to appear from UART
function wait(pattern)
   local line
   repeat
      line = uart.read( UARTID, '*l')
      if verbose == 1 then print(line) end
   until string.find(line,pattern)
   return line
end

-- Send commands, separated by newline,
-- and wait for OK responses to each
function cmd(cmd_str)
   local line
   for line in cmd_str:gmatch("[^\r\n]+") do
      send(line)
      wait("^OK")
      tmr.delay( 0, 5000 )
   end
end

function setup()
   poweron()
   -- setup serial to 115200 HW Flow control
   send('AT+IPR=115200')
   tmr.delay( 0, 50000 )
   send('AT+IFC=2,2')
   tmr.delay( 0, 50000 )
   -- dont wait for response, serial speed may be already 115200 and we are sending 9600
   
   uart.setup( UARTID, 115200, 8, uart.PAR_NONE, uart.STOP_1 )
   uart.set_flow_control( UARTID, uart.FLOW_RTS + uart.FLOW_CTS )
   tmr.delay( 0, 50000 )
	
   -- send pin
   cmd([[
AT+IFC=2,2
ATE0
AT
AT+CPIN=]]..options.pin_code)
   -- wait for network
   local response
   local found
   repeat
      tmr.delay( 0, 500000 )
      send("AT+COPS?")
      response = wait("^+COPS")
      found,_,network = string.find(response,'%+COPS: 0,0,"([^"]*)"')
   until found
   print("Registered to network "..network)
   --Wait for final confirmation
   wait("^Call Ready")
   print("GSM Ready")
end

function start_gprs()
   -- GPRS settings
   cmd([[
AT+SAPBR=3,1,"CONTYPE","GPRS"
AT+SAPBR=3,1,"APN","internet"
AT+SAPBR=1,1
]])
   print("GPRS enabled")
end
