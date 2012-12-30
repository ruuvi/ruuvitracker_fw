require('gsm')

gps_enabled = 0
gps_uart = 1

function gps_enable()
	uart.setup( gps_uart, 115200, 8, uart.PAR_NONE, uart.STOP_1 )
	gsm_cmd('AT+CGPSPWR=1')
	gsm_cmd('AT+CGPSRST=0')
	gps_enabled = 1
end

function gps_monitor()
	if gps_enabled==0 then gps_enable() end
	
	
	while 1 == 1 do
		str = uart.read(gps_uart,'*l')
		if str:find("^%$GPRMC") then
				local time, status, latitude, ns_indicator, longitude,
			  	ew_indicator, speed = str:match("^%$GPRMC,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*)")
				print("Time: " ..time)
				print("Status: "..status)
		end
		if str~="" then print(str) end
		
		q = uart.read(0,'*l',uart.NO_TIMEOUT)
		if q == "q" then break end
	end
end
