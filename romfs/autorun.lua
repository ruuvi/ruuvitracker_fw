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
