require('gsm')

function enable_gps()
	uart.setup( 1, 115200, 8, uart.PAR_NONE, uart.STOP_1 )
	gsm_cmd('AT+CGPSPWR=1')
	gsm_cmd('AT+CGPSRST=0')
end
