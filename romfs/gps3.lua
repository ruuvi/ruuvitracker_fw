-- Example script for getting a GPS location from
-- Simcom 908 with RuuviTracker GPS api.
--
-- Author: Tomi Hautakoski


-- First example. Power up device

print("Powering up GSM\n")
gsm.set_power_state(gsm.POWER_ON)
print("GSM powered\n")

-- Second example. Check if pin is disabled
print("Checking if PIN is required...\n")
if gsm.is_pin_required() then
    print("Sending PIN")
    gsm.send_pin( "0000" ) -- Change your pin code here
end


print("Waiting for GSM to be registered to the network)\n")
repeat ruuvi.delay_ms(100) until gsm.is_ready()
print("GSM ready and registed to network\n")

print("Sending GPS power-on command, waiting for response...\n")
print(gsm.cmd("AT+CGPSPWR=1"))

print("Sending GPS cold reset command, waiting for response...\n")
print(gsm.cmd("AT+CGPSRST=1"))

print("Setup done, waiting for GPS data from UART 2...\n")
repeat ruuvi.delay_ms(1000) until gps.has_fix()
