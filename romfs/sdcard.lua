--[[--
Use with:
sd = require "sdcard"

sd.enable() return boolean if the operation was successfull or not (fails if no card inserted)
sd.disable() will always succeed
sd.is_inserted() returns boolean
sd.is_enabled() return the enable status, will also return failure if card not inserted. 
                NOTE! This does not actually tell you if the card is ready/working/whatnot, it seems eLua
                does not like having the card come and go, you should never try to access the card when
                it's not enabled, this will likely mess up eLua/FatFS and prevent you accessing the card
                until next reboot...
--]]--

local sdmodule = {}

-- Board specifig config
if pd.board() == "RUUVIC1"
then
   local inserted_pin = pio.PC_10
   local enable_pin   = pio.PC_8
end


function sdmodule.is_inserted()
    -- Make sure the pin is in correct mode
    pio.pin.setdir(pio.INPUT, enable_pin)
    pio.pin.setpull(pio.PULLUP, inserted_pin)
    return (pio.pin.getval(inserted_pin) == 0)
end

function sdmodule.is_enabled()
    -- Make sure the pin is in correct mode
    if (not sdmodule.is_inserted())
    then
        return false
    end
    return (pio.pin.getval(sdmodule.enable_pin) == 1)
end

function sdmodule.disable()
    -- let the pull-down resistor on the board shut down the regulator
    pio.pin.setdir(pio.INPUT, enable_pin)
    pio.pin.setpull(pio.NOPULL, enable_pin)
    -- And disable pullup on the sense pin as well, just in case it would matter at all
    pio.pin.setpull(pio.NOPULL, inserted_pin)
end

function sdmodule.enable()
    if (not sdmodule.is_inserted())
    then
        return false
    end
    pio.pin.setdir(pio.OUTPUT, enable_pin)
    pio.pin.sethigh(enable_pin)
end


return sdmodule

