-- Example script for getting a GPS location from
-- Simcom 908 with RuuviTracker GPS api.
--
-- Author: Tomi Hautakoski

print("Powering up GSM\n")
gsm.set_power_state(gsm.POWER_ON)
print("GSM powered\n")

print("Checking if PIN is required...\n")
if gsm.is_pin_required() then
    print("Sending PIN")
    gsm.send_pin( "0000" ) -- Change your pin code here
end

print("Wait for modem to be Ready (On the network)\n")
repeat ruuvi.delay_ms(100) until gsm.is_ready()
print("GSM ready and registed to network\n")

print("Sending GPS power-on command, waiting for response...\n")
gsm.cmd("AT+CGPSPWR=1")

print("Sending GPS cold reset command, waiting for response...\n")
gsm.cmd("AT+CGPSRST=1")

print("Setup GPS UART port\n")
uart.setup(2, 115200, 8, uart.PAR_NONE, uart.STOP_1)
timeout = 50e3 -- Timeout 100ms for messages to arrive

print("Setup done, waiting for GPS data from UART 2...\n")
while true do
    local str = uart.read(2, '*l', timeout)
    if str ~= "" then
	print(str)
    end
end

