rts = pio.PB_14
cts = pio.PB_13
cdc = pio.PB_0
ring = pio.PB_1
dtr = pio.PC_5
power = pio.PC_4

gsm_verbose = 0

function gsm_poweron()
	print("GSM Power on..")
	pio.pin.setdir(pio.OUTPUT, power)
	pio.pin.setlow(power) 
	tmr.delay( 0, 500000 )
	pio.pin.sethigh(power)
	tmr.delay( 0, 500000 )
	uart.setup( 2, 9600, 8, uart.PAR_NONE, uart.STOP_1 )
	uart.set_flow_control( 2, uart.FLOW_RTS + uart.FLOW_CTS )
	uart.write(2,"AT\r")
	tmr.delay( 0, 50000 )
end

function gsm_send(str)
	if gsm_verbose == 1 then print(str) end
	uart.write(2, str.."\r")
end

function gsm_wait(pattern)
	local line
	repeat
		line = uart.read( 2, '*l')
		if gsm_verbose == 1 then print(line) end
	until string.find(line,pattern)
	return line
end

function gsm_cmd(cmd_str)
	local line
	for line in cmd_str:gmatch("[^\r\n]+") do
		gsm_send(line)
		gsm_wait("^O")
		tmr.delay( 0, 50000 )
	end
end

function gsm_setup()
	gsm_poweron()
--setup serial to 115200 HW Flow control
	gsm_send('AT+IPR=115200')
	tmr.delay( 0, 50000 )
	gsm_send('AT+IFC=2,2')
	tmr.delay( 0, 50000 )
	--dont wait for response, serial speed may be already 115200 and we are sending 9600

	uart.setup( 2, 115200, 8, uart.PAR_NONE, uart.STOP_1 )
	uart.set_flow_control( 2, uart.FLOW_RTS + uart.FLOW_CTS )
	tmr.delay( 0, 50000 )
	
	--send pin
	gsm_cmd([[
AT+IFC=2,2
ATE0
AT
AT+CPIN=0000
]])
	--wait for network
	local response
	local found
	repeat
		tmr.delay( 0, 500000 )
		gsm_send("AT+COPS?")
		response = gsm_wait("^+COPS")
		found,_,network = string.find(response,'%+COPS: 0,0,"([^"]*)"')
	until found
	print("Registered to network "..network)
	--Wait for final confirmation
	gsm_wait("^Call Ready")
	print("GSM Ready")
end

function gsm_start_gprs()
	--GPRS settings
	gsm_cmd([[
AT+SAPBR=3,1,"CONTYPE","GPRS"
AT+SAPBR=3,1,"APN","internet"
AT+SAPBR=1,1
]])
	print("GPRS enabled")
end
