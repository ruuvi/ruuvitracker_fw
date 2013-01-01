term.clrscr()
print("Starting RuuviTracker eLua FW")
print("Board: " .. pd.board() )

print("Set green led ON\n")

if pd.board() == "STM32F4DSCY" then
	led = pio.PD_12
elseif pd.board() == "RUUVIA" then
	led = pio.PC_14
end
	
pio.pin.setdir( pio.OUTPUT, led )
pio.pin.sethigh( led )

require('gsm')
gsm_setup()
gsm_start_gprs()

require('gps')
gps_looper()