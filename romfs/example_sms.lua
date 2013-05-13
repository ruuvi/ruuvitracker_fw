-- Example SMS handling
-- Author: Seppo Takalo

flag_run = true

-- Function that receives message
-- index is memory index inside of gsm modem
-- It would be wise to delete message after handling
function sms_handler(index)
   local msg = gsm.read_sms(index)
   print("Received message from", msg.from)
   print(msg.text)
   gsm.delete_sms(index)
   flag_run = false -- Notify main loop to stop
end

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
