-- Example SMS handling
-- Author: Seppo Takalo
-- run with: lua /rom/example_sms.lc

flag_run = true

-- Function that receives message
-- index is memory index inside of gsm modem
-- It would be wise to delete message after handling
function sms_handler(index)
   print("new message in index ", index)
   local msg = gsm.read_sms(index)
   print("Received message from", msg.from)
   print(msg.text)
   gsm.delete_sms(index)
   flag_run = false -- Notify main loop to stop
end

-- Enable the module
print("Powering up gsm\n")
gsm.set_power_state(gsm.POWER_ON)

-- Second example. Check if pin is disabled
if gsm.is_pin_required() then
   gsm.send_pin( "0000" ) -- Change your pin code here
end

-- Wait for modem to be Ready (On the network)
ready_loop_started = ruuvi.millis()
while(not gsm.is_ready())
do
    if ((ruuvi.millis() - ready_loop_started) > 5000)
    then
        -- timeout
        break
    end
end

if (not gsm.is_ready())
then
    print("GSM module not ready, please try again")
else
    -- SMS is a virtual interrupt inside of eLua.
    -- Register handler for it
    cpu.set_int_handler(cpu.INT_GSM_SMS, sms_handler)
    
    
    print("Ready to receive!\nPlease send me a SMS!")
    
    -- Nothing to do for me
    -- except keep eLua running
    while flag_run do
       ruuvi.delay_ms(1000)
    end
    
    -- Unregister handler
    cpu.set_int_handler(cpu.INT_GSM_SMS, nil)
end
