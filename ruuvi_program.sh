#!/bin/sh
#
# Programming script for RuuviTracker
# Author: Seppo Takalo

source ./ruuvi.conf

if [ "$BOARD" == "RUUVIA" ]; then
    BINARY="elua_lua_stm32f415rg.bin"
elif [ "$BOARD" == "RUUVIB1" ]; then
    BINARY="elua_lua_stm32f407vg.bin"
else
    echo "ERROR, Unknown board"
    exit 1
fi

dfu-util -i 0 -a 0 -D $BINARY -s 0x08000000:leave

