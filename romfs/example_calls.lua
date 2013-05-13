-- Example handler for incoming call
-- Author: Seppo Takalo

flag_run = true
function call_handler()
   local number = gsm.get_caller()
   print("Incoming call from", number)
   flag_run = false
end

-- Incoming call is virtual interrupt in eLua
-- We need to register handler for it
cpu.set_int_handler(cpu.INT_GSM_CALL, call_handler)


print("Call handler registered. Give me a call\n")

-- NOTE: eLua interrupts does not work if eLua is stopped.
-- Keep running until call received
while flag_run do
   ruuvi.delay_ms(100)
end

print("Done")
