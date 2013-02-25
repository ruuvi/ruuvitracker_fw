require('config')  -- Load config
require('logging')

module('gsm', package.seeall)  -- Define module 'gsm'

local power_pin
local dtr_pin
local status_pin
local enable_voltage_pin
local cts_pin

if pd.board() == "RUUVIA" then
   power_pin = pio.PC_4
   dtr_pin = pio.PC_5
elseif pd.board() == "RUUVIB1" then
   power_pin = pio.PE_2
   dtr_pin = pio.PC_14
   status_pin = pio.PE_12
   enable_voltage_pin = pio.PC_15
   cts_pin = pio.PD_4
end


local logger = Logger:new('gsm')

local UARTID
if pd.board() == "RUUVIA" then
   UARTID = 2
elseif pd.board() == "RUUVIB1" then
   UARTID = 1
end

local gsm_timeout = 10e6 -- Default timeout 10s
local timer = firmware.timers.gsm_timer
local timer_hz = 1000  -- Slowest possible

-- Modem Status
local Status = {
   power_down   = 0,
   start        = 1,
   ask_pin      = 2,
   find_network = 3,
   on_network   = 4,
   start_gprs   = 5,
   ready        = 6,
   idle         = 7,
   sleep        = 8,
   error        = -1
}
local status = Status.power_down

idle_allowed = false
sleep_allowed = false

-- GSM handler co-routine
local function state_machine()
   -- Adjust my clock
   tmr.setclock(timer, timer_hz)
   while true do
      -- State machine
      if status == Status.power_down then
	 logger:info("Powering up")
	 setup_hw()
	 power_toggle()
	 status = Status.start
      elseif status == Status.start then
	 -- Modem booting
	 if wait("RDY", 10e6) == false then
	    switch_gsm_9600_to_115200()
	    status = Status.error -- Go through error state
	 else
	    logger:info("'RDY'")
	    -- Boot ok, next status
	    status = Status.ask_pin
	    -- Wait modem to boot
	    wait("+CFUN: 1", 5e6)
	    -- Setup HW flow, and echo-off
	    cmd("AT+IFC=2,2\nATE0")
	    logger:info("HW flow control set")
	    enable_sleep()
	    logger:info("Sleep mode 1 enabled")
	 end
      elseif status == Status.ask_pin then
	 send("AT+CPIN?")
	 local response = wait("^+CPIN")
	 wait("^OK")
	 if response:find("SIM PIN") then -- pin required
	    cmd("AT+CPIN="..config.gsm.pin_code) -- Send pin
	    logger:info("pin ok")
	    logger:info("disabling pin query")
	    cmd('AT+CLCK="SC",0,"'..config.gsm.pin_code..'"') -- Remove pin query
	 else
	    logger:info("pin not required")
	 end
	 -- TODO: Handle Errors, PUK, etc..
	 status = Status.find_network
      elseif status == Status.find_network then
	 logger:info("Waiting for network")
	 local response = wait("^Call Ready", 50e6) -- Wait 50s for GSM network (max delay)
	 if response == false then
	    status = Status.error -- Did not find network
	    logger:error("No network")
	 else
	    status = Status.on_network
	 end
      elseif status == Status.on_network then
	 send("AT+COPS?")
	 -- Sometimes there is \n in response. eg: +COPS: \n0,0,"operator"
	 local found, _, network = string.find(wait("0,0"),'0,0,"([^"]*)"')
	 wait("^OK")
	 if found then logger:info("Registered to network " ..network)
	 else logger:warn("Could not find network from +COPS")
	 end
	 status = Status.start_gprs
      elseif status == Status.start_gprs then
	 logger:info("Enabling GPRS")
	 start_gprs()
	 logger:info("GPRS up")
	 status = Status.ready
      elseif status == Status.error then
	 logger:warn("Error state, resetting modem")
	 power_toggle() -- Shut down modem
	 delay(5e6)     -- Wait 5s for modem to be shut down
	 setup_hw()     -- Reset Serial port and empty buffers
	 send('AT')     -- Test if not really shut down
	 if wait("^OK") ~= false then -- Power "mode" is reversed
	    logger:debug("GSM Still on. Powering down")
	    delay(5e6)     -- Wait 5s
	    power_toggle() -- Need to toggle power again to shut down
	    delay(5e6)     -- Wait 5s again
	 else
	    logger:debug("GSM shut down")
	 end
	 status = Status.power_down
      elseif status == Status.ready then
	 -- Nothing to do
	 if sleep_allowed then
	    cmd("AT+CFUN=0") -- Go to minimum functionality mode
	    sleep()
	    status = Status.sleep
	 elseif idle_allowed then
	    sleep()
	    status = Status.idle
	 end
      elseif status == Status.idle then
	 if not idle_allowed then
	    end_sleep()
	    status = Status.ready
	 end
      elseif status == Status.sleep then -- Minimum functionality mode, Radio=off
	 if not sleep_allowed then
	    end_sleep()
	    cmd("AT+CFUN=1")
	    status = Status.find_network
	 end
      else
	 --Unknown status
	 logger:error("Unknown status")
	 status = Status.error
      end
      coroutine.yield()
   end
end
-- Create co-routine from state machine
handler = coroutine.create(state_machine)


-- Wait for GSM ready condition
-- Use from inside of a co-routine
function wait_ready()
   while status ~= Status.ready do
      coroutine.yield()
   end
end

-- Test if GSM is ready
-- Use this outside of co-routine
function is_ready()
   if status == Status.ready then
      return true
   else
      return false
   end
end

-- Wait function
-- Use this inside of co-routine
function delay(time)
   local counter = tmr.start(timer)
   local delta = 0
   repeat
      coroutine.yield() -- Give time to other rountines
      delta = tmr.gettimediff(timer, counter, tmr.read(timer))
   until delta > time
end

-- Toggle power pin
function power_toggle()
   pio.pin.setdir(pio.OUTPUT, power_pin)
   pio.pin.setlow(power_pin)
   delay(1e6) -- Hold power low for 1s
   pio.pin.sethigh(power_pin)
end

-- Send string to GSM UART
-- append extra "\r" to serial port
function send(str)
   logger:debug("Send:"..str)
   uart.write(UARTID, str.."\r")
end

-- Read raw bytes from GSM
function read_raw(nbytes)
   local len = tonumber(nbytes)
   local data = ""
   repeat
      local buf = uart.read(UARTID,len)
      len = len - buf:len()
      data = data .. buf
   until len == 0
   return data
end


-- Wait for string pattern to appear from UART
-- Use only inside a co-routine (this yields)
function wait(pattern, timeout)
   local timeout = timeout or gsm_timeout
   local line
   local counter = tmr.start(timer)
   repeat
      line = uart.read(UARTID, '*l', 10e3) -- Wait 10ms for answer
      if line ~= "" then logger:debug(line) end
      if string.find(line, pattern) then return line end -- Found response
      if line == "" then coroutine.yield() end -- No response yet, waiting
   until tmr.gettimediff(timer, counter, tmr.read(timer)) > timeout -- Timeout
   return false
end

-- Send commands, separated by newline,
-- and wait for OK responses to each
-- Use only inside a co-routine (this yields)
function cmd(cmd_str)
   local line
   for line in cmd_str:gmatch("[^\r\n]+") do
      repeat
	 send(line)
      until wait("^OK") ~= false -- Resend CMD if OK not received
   end
end

function setup_hw()
   pio.pin.setdir(pio.OUTPUT, dtr_pin)
   pio.pin.setlow(dtr_pin) -- Sleep mode 1 is controlled with DTR pin (HIGH=sleep, LOW=ready)
   if pd.board() == "RUUVIB1" then
      pio.pin.setdir(pio.OUTPUT, enable_voltage_pin)
      pio.pin.setlow(enable_voltage_pin)
      pio.pin.setdir(pio.INPUT, status_pin)
      pio.pin.setdir(pio.OUTPUT, cts_pin)
      pio.pin.setlow(cts_pin)
   end
   uart.setup(UARTID, 115200, 8, uart.PAR_NONE, uart.STOP_1)
   uart.set_flow_control(UARTID, uart.FLOW_RTS + uart.FLOW_CTS)
end

function switch_gsm_9600_to_115200()
   uart.setup(UARTID, 9600, 8, uart.PAR_NONE, uart.STOP_1)
   uart.set_flow_control(UARTID, uart.FLOW_NONE)
   delay(1e6) -- Wait 50ms
   send('AT')  -- This is for autobauding to catch
   send('AT')
   delay(5e6)
   -- Setup serial to 115200 HW flow control
   send('AT+IPR=115200')
   delay(5e6) -- Wait 5s for modem
   -- Don't wait for response, serial speed may already be 115200 and we are sending 9600
end

function start_gprs()
   -- GPRS settings
   return cmd([[
      AT+SAPBR=3,1,"CONTYPE","GPRS"
      AT+SAPBR=3,1,"APN","]]..config.gsm.apn..[["
      AT+SAPBR=1,1]])
   end

-- Send SMS message to a number
function send_sms(number, text)
   cmd("AT+CMGF=1")
   send('AT+CMGS="'..number..'"')
   wait(">")  -- This is undocumented but required
   send(text..string.char(26)) -- Message + CTRL-Z
   wait("^OK", 5e6)  -- Wait 5s to complete
end

-- Enable Sleep mode 1
function enable_sleep()
   cmd("AT+CSCLK=1")
end

-- Go to sleep mode
function sleep()
   logger:info("Entering sleep mode")
   pio.pin.sethigh(dtr_pin)
end

-- Stop sleeping
function end_sleep()
   logger:info("Waking up from sleep")
   pio.pin.setlow(dtr_pin)
   delay(100e3) -- Wait 100ms to wake up
end

