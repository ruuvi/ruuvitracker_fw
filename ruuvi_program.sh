#st-flash write elua_lua_stm32f407vg.bin 0x08000000
dfu-util -i 0 -a 0 -D elua_lua_stm32f415rg.bin -s 0x08000000
