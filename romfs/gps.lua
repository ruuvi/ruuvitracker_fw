require('gsm')

-- Start package 'gps'
module(..., package.seeall)

local enabled = 0
local gps_uart = 1

timeout = 100

is_fixed=false

event={}

function enable()
   uart.setup( gps_uart, 115200, 8, uart.PAR_NONE, uart.STOP_1 )
   gsm.cmd('AT+CGPSPWR=1')
   gsm.cmd('AT+CGPSRST=0')
   enabled = 1
end

local function parse_rmc(line)
   local time, status, latitude, ns_indicator, longitude,
   ew_indicator, speed_knots, heading, date = line:match("^%$GPRMC,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*)")
   if status == "A" then is_fixed=true
   else is_fixed=false return end

   -- Calculate speed
   local speed_ms = nil
   if speed_knots then
	   speed_ms = 0.51444 * speed_knots
   end

   -- Calculate time and data
   local h,m,s = time:match("(..)(..)(......)") -- GPRMC time is hhmmss.sss
   local dd,mm,yy = date:match("(..)(..)(..)") -- GPRMC date is ddmmyy
   time = h..':'..m..':'..s
   date = '20'..yy..'-'..mm..'-'..dd
   local timestamp = date .. 'T' .. time .. 'Z'
   --Get first timestamp as a session_code
   session_code = session_code or timestamp
   -- Fill coordinates
   latitude = latitude..','..ns_indicator
   longitude = longitude..','..ew_indicator

   print("Time: " ..time)
   print("Date: " ..date)
   print("Timestamp: " ..timestamp)
   print("Status: "..status)
   print("Lat:" ..latitude)
   print("Lon:" ..longitude)
   print("Speed:" .. speed_knots .. "knots")
   print("Heading:" .. heading)

   -- Fill the event object
   event.latitude = latitude
   event.longitude = longitude
   event.session_code = session_code
   event.speed = speed_ms
   event.heading = heading
   event.time = timestamp
   
end
local function parse_gga(line)
   local nsatellites = line:match("^%$GPGGA,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*)")
   if tonumber(nsatellites) ~= nil then
      event.satellite_count = tonumber(nsatellites)
   else
      event.satellite_count = 0
   end
   print("Tracking " ..event.satellite_count.." satellites")
end

function process()
   if enabled==0 then enable() end
   
   is_fixed = false
   event = {}
   while true do
      local str = uart.read(gps_uart,'*l', timeout)
      
      if str:find("^%$GPRMC") then
	 parse_rmc(str)
      elseif str:find("^%$GPGGA") then
	 parse_gga(str)
      end

    --  if str~="" then print(str) end
      if str=="" then break end
   end
end
