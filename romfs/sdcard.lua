--[[--
Use with:
sd = require "sdcard"

sd.enable() return boolean if the operation was successfull or not (fails if no card inserted)
sd.disable() will always succeed
sd.is_inserted() returns boolean

--]]--

local sdmodule = {}

-- Board specifig config
if pd.board() == "RUUVIC1"
then
   local inserted_pin = pio.PC_10
   local enable_pin   = pio.PC_8
end


local function is_inserted()
    -- Make sure the pin is in correct mode
    pio.pin.setdir(pio.INPUT, enable_pin)
    pio.pin.setpull(pio.PULLUP, inserted_pin)
    return (pio.pin.getval(inserted_pin) == 0)
end

local function disable()
    -- let the pull-down resistor on the board shut down the regulator
    pio.pin.setdir(pio.INPUT, enable_pin)
    pio.pin.setpull(pio.NOPULL, enable_pin)
    -- And disable pullup on the sense pin as well, just in case it would matter at all
    pio.pin.setpull(pio.NOPULL, inserted_pin)
end

local function enable()
    if (not is_inserted())
    then
        return false
    end
    pio.pin.setdir(pio.OUTPUT, enable_pin)
    pio.pin.sethigh(enable_pin)
end

sdmodule.enable = enable
sdmodule.is_inserted = is_inserted
sdmodule.disable = disable

return sdmodule

