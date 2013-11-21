--[[--
Use with:
leds = require "led"
leds.green_led:on()
leds.green_led:off()
--]]--


local ledmodule = {}

local Led = {}
function Led:on()
   pio.pin.setdir(pio.OUTPUT, self.pin)
   pio.pin.sethigh(self.pin)
end
function Led:off()
   pio.pin.setdir(pio.OUTPUT, self.pin)
   pio.pin.setlow(self.pin)
end
function Led:new(pin)
   o = {}
   o.pin = pin
   setmetatable(o, self)
   self.__index = self
   return o
end

ledmodule.Led = Led

if pd then
    if pd.board() == "STM32F4DSCY" then
       -- TODO: add leds for this board
    elseif pd.board() == "RUUVIA" then
       green_led = Led:new(pio.PC_14)
       red_led   = Led:new(pio.PC_15)
    elseif pd.board() == "RUUVIB1" then
       green_led = Led:new(pio.PE_14)
       red_led   = Led:new(pio.PE_15)
    elseif pd.board() == "RUUVIC1" then
       green_led = Led:new(pio.PB_9)
       red_led   = Led:new(pio.PB_8)
    end
    ledmodule.green_led = green_led
    ledmodule.red_led = red_led
end


return ledmodule
