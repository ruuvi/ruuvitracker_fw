-- Example script for getting a GPS location from
-- Simcom 908 with RuuviTracker GPS api.
--
-- Author: Tomi Hautakoski


-- First example. Power up device

print("Powering up GSM\n")
gsm.set_power_state(gsm.POWER_ON)

repeat ruuvi.delay_ms(1000) until gsm.flag_is_set(gsm.GPS_READY)

print("Sending GPS power-on command, waiting for response...\n")
gsm.cmd("AT+CGPSPWR=1")

gsm.cmd("AT+CGPSOUT=255")

print("Sending GPS warm reset command\n")
gsm.cmd("AT+CGPSRST=1")

print("Setup done, waiting for GPS Fix!\n")
repeat ruuvi.delay_ms(1000) until gps.has_fix()

print("GPS Got FIX!")
