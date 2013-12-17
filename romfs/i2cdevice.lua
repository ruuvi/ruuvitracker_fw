--[[--
Helper baseclass for interfacing with I2C devices, you can use it as is,
but much better idea is to subclass it for each device.

This is basically port of my Arduino I2C device library: https://github.com/rambo/i2c_device

Usage (without subclassing)

i2cdev = require "i2cdevice"
md = i2cdev.Device:new(0x1d)
md:present() -- returns true/false depending on whether the slave ACKed or not
md:dump_registers(0x0, 0x31) -- this will do a bunch of sequential single byte reads
md:write(reg, value)
md:read(reg, amount) -- amount is optional and defaults to 1
md:read_modify_write(reg, mask, value, i2cdev.OR) -- the last param is optional and defaults to OR, C equivalent being: new_value = (old_value & mask) | given_value
md:set_bit(reg, bitno, ...) -- just like bit.set can handle multiple positions in one call, these are probably much more handy than the old read_modify_write method above
md:clear_bit(reg, bitno, ...) -- just like bit.set can handle multiple positions in one call


For usage with subclassing see the mma8652fc module

--]]--
local i2cdevmod = {}
require "yaci"

i2cdevmod.debug = true

-- judging by platform_i2c.c I2C1 (which is the only one on RuuviTracker) is the one to use and it's the zero key in the map...
i2cdevmod.default_bus = 0
i2cdevmod.default_speed = i2c.FAST
i2cdevmod.pullups = {}
-- Pins for I2C1 on revc1 (must be defined like this since Lua for some reason indexes arrays starting from 1 instead of 0 like every other language...)
i2cdevmod.pullups[0] = { pio.PB_6,  pio.PB_7 }

-- Constants for read-modify-write operations
i2cdevmod.AND = 0
i2cdevmod.OR  = 1
i2cdevmod.XOR = 2

-- Checks the input type and casts to number, string inputs are supposed to be in hexadecimal
local function number_cast(inval)
    if (type(inval) == "string")
    then
        return tonumber(inval, 16)
    end
    return inval
end

-- Enable pull-ups (if defined) for given bus-id, returns tue/false depending on whether pull-ups were defined
local function enable_pullups(bus_id)
    if (i2cdevmod.pullups[self.bus_id])
    then
        for i,pin in ipairs(i2cdevmod.pullups[bus_id])
        do
             pio.pin.setpull(pio.PULLUP, pin)
        end
        return true
    end
    return false
end



local Device = Object:subclass("Device")
-- Constructor, takes device address (7-bit!) and optionally bus-id, string typed addresses are supposed to be in hexadecimal
function Device:init(address, bus_id, bus_speed)
    self.address = number_cast(address)
    if (self.address > 127)
    then
        error("Use 7-bit address")
    end
    if (bus_id == nil)
    then
        self.bus_id = i2cdevmod.default_bus
    else
        self.bus_id = bus_id
    end
    if (bus_speed == nil)
    then
        self.bus_speed = i2cdevmod.default_speed
    else
        self.bus_speed = bus_speed
    end
    self:bus_init()
end

-- Re-initialize the I2C bus just in case you need to switch speeds or something
function Device:bus_init()
    i2c.setup(self.bus_id, self.bus_speed)
end

-- Reads amount bytes from register reg
-- returns the data read as i2c.read returns it (use data:byte(index) to get the actual byte values, remember that Lua indices start from 1)
-- Reading more than one byte at a time (at least from the onboard accelerometer) seems to hang eLua...
function Device:read(reg, amount)
    local bus_id = amount or 1 
    reg = number_cast(reg)
    -- TODO: this might throw an error (or it might not, documentation is not clear), catch it...
    i2c.start(self.bus_id)
    -- I suppose the direction here will set the R/W bit in the address, the eLua documentation is not 100% clear on this.
    local acked = i2c.address(self.bus_id, self.address, i2c.TRANSMITTER)
    if (not acked)
    then
        -- No such address, abort
        if (i2cdevmod.debug)
        then
            print(string.format("ERROR: dev 0x%02x did not ACK in TRANSMITTER mode", self.address))
        end
        i2c.stop(self.bus_id)
        return nil
    end
    local wrote = i2c.write(self.bus_id, reg)
    if (not(wrote == 1))
    then
        -- Something went wrong, abort
        if (i2cdevmod.debug)
        then
            print(string.format("ERROR: dev 0x%02x writing 0x%02x returned %d", self.address, reg, wrote))
        end
        i2c.stop(self.bus_id)
        return nil
    end
    -- Stop and start, the wrong way, but I get no ACK from the slave when doing repeated start
    i2c.stop(self.bus_id)
    i2c.start(self.bus_id)
    -- Repeated start, the correct way to do this
    --i2c.start(self.bus_id)
    -- I suppose the direction here will set the R/W bit in the address, the eLua documentation is not 100% clear on this.
    local acked = i2c.address(self.bus_id, self.address, i2c.RECEIVER)
    if (not acked)
    then
        -- Just in case the device went away at this point
        if (i2cdevmod.debug)
        then
            print(string.format("ERROR: dev 0x%02x did not ACK in RECEIVER mode", self.address))
        end
        i2c.stop(self.bus_id)
        return nil
    end
    local data = i2c.read(self.bus_id, amount)
    i2c.stop(self.bus_id)
    -- Remember to use data:byte(index) to get the actual byte value 
    return data
end

-- Writes to register reg, takes N data arguments just like i2c.write
-- returns the number of data bytes sent (the register is not in this count)
function Device:write(reg, ...)
    local reg = number_cast(reg)
    -- TODO: this might throw an error (or it might not, documentation is not clear), catch it...
    i2c.start(self.bus_id)
    -- I suppose the direction here will set the R/W bit in the address, the eLua documentation is not 100% clear on this.
    local acked = i2c.address(self.bus_id, self.address, i2c.TRANSMITTER)
    if (not acked)
    then
        -- No such address, abort
        if (i2cdevmod.debug)
        then
            print(string.format("ERROR: dev 0x%02x did not ACK in TRANSMITTER mode", self.address))
        end
        i2c.stop(self.bus_id)
        return nil
    end
    -- First write just the register
    local wrote = i2c.write(self.bus_id, reg)
    if (not(wrote == 1))
    then
        -- Something went wrong, abort
        if (i2cdevmod.debug)
        then
            print(string.format("ERROR: dev 0x%02x writing 0x%02x returned %d", self.address, reg, wrote))
        end
        i2c.stop(self.bus_id)
        return nil
    end
    -- then the rest of it
    local wrote = i2c.write(self.bus_id, unpack(arg))
    i2c.stop(self.bus_id)
    return wrote
end

-- Dumps the contents of given register range one-by-one
-- Does this naively just in case register address auto-increment is not enabled/supported on the device
function Device:dump_registers(start_reg, end_reg)
    for reg=start_reg,end_reg
    do
        local value = self:read(reg, 1)
        if (value == nil)
        then
            print(string.format("dev 0x%02x reg 0x%02x READ FAILED", self.address, reg))
        else
            -- Unfortunately there is no built-bin number->binary string formatter or even a helper function
            print(string.format("dev 0x%02x reg 0x%02x value 0x%02x", self.address, reg, value:byte(1))) -- Remember Lua indices start from 1 (http://www.youtube.com/watch?v=IuGjtlsKo4s#t=13)
        end
    end
end

-- Do a masked read-modify-write, to twiddle register bits
-- Returns the new value, or nil in case of trouble
function Device:read_modify_write(reg, mask, value, operand)
    local operand = operand or i2cdevmod.OR
    local reg_value = self:read(reg, 1)
    if (ref_value == nil)
    then
        return nil
    end
    local reg_new_value = reg_value
    if (openrand == i2cdevmod.OR)
    then
        new_value = bit.bor(bit.band(value, mask), value)
    end
    if (openrand == i2cdevmod.AND)
    then
        new_value = bit.band(bit.band(value, mask), value)
    end
    if (openrand == i2cdevmod.XOR)
    then
        new_value = bit.bxor(bit.band(value, mask), value)
    end
    local wrote self:write(reg, reg_new_value)
    if (wrote == nil)
    then
        return nil
    end
    return reg_new_value
end

-- Wrapper for bit.set to do the operation on a device register
-- Returns the new value, or nil in case of trouble
function Device:set_bit(reg, ...)
    local reg_value = self:read(reg, 1)
    if (ref_value == nil)
    then
        return nil
    end
    local reg_new_value = bit.set(reg_value, unpack(arg))
    local wrote self:write(reg, reg_new_value)
    if (wrote == nil)
    then
        return nil
    end
    return reg_new_value
end

-- Wrapper for bit.clear to do the operation on a device register
-- Returns the new value, or nil in case of trouble
function Device:clear_bit(reg, ...)
    local reg_value = self:read(reg, 1)
    if (ref_value == nil)
    then
        return nil
    end
    local reg_new_value = bit.clear(reg_value, unpack(arg))
    local wrote self:write(reg, reg_new_value)
    if (wrote == nil)
    then
        return nil
    end
    return reg_new_value
end

-- Check ACK from the device.
function Device:present()
    i2c.start(self.bus_id)
    local acked = i2c.address(self.bus_id, self.address, i2c.RECEIVER)
    i2c.stop(self.bus_id)
    return acked
end

i2cdevmod.Device = Device

-- Address space scan helper
function i2cdevmod.address_scan(bus_id, bus_speed)
    -- this works for numbers, but if we need to distinguish between false and nil we need a different approach
    local bus_id = bus_id or i2cdevmod.default_bus 
    local bus_speed = bus_speed or i2cdevmod.default_speed
    local speed = i2c.setup(bus_id, bus_speed)
    print(string.format("Scanning bus %d at speed %s", bus_id, speed))
    for addr=0,127
    do
        i2c.start(bus_id)
        local acked = i2c.address(bus_id, addr, i2c.RECEIVER)
        if (acked)
        then
            print(string.format("FOUND device 0x%02x", addr))
        end
        i2c.stop(bus_id)
    end
end


return i2cdevmod