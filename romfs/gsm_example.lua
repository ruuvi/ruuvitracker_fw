

-- Example script for handling GSM modem
-- Simcom 908 with RuuviTracker gsm api.
--
-- Author: Seppo Takalo


-- First example. Power up device

print("Powering up gsm\n")
gsm.set_power_state(gsm.POWER_ON)

-- Modem now boots automatically to network, if PIN query is disabled.

-- Second example. Check if pin is disabled
if gsm.is_pin_required() then
   gsm.send_pin( "0000" ) -- Change your pin code here
end


-- Wait for modem to be Ready (On the network)
repeat ruuvi.delay_ms(100) until gsm.is_ready()
print("GSM ready and on network")


-- Third example: Send SMS message
gsm.send_sms('+35850123123', "RuuviTracker Booted\nYeah!")  -- Change receivers number

-- Last example: shut down gsm
gsm.set_power_state(gsm.POWER_OFF)

