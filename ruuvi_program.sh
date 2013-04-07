#!/bin/bash
#
# Programming script for RuuviTracker
# Author: Seppo Takalo

source ./ruuvi.conf

function wait_for_dfu() {
    while ! dfu-util -l |grep "Found DFU">/dev/null;do
	sleep 0.1
    done
}

if [ "$BOARD" == "RUUVIA" ]; then
    BINARY="elua_lua_stm32f415rg.bin"
elif [ "$BOARD" == "RUUVIB1" ]; then
    BINARY="elua_lua_stm32f407vg.bin"
else
    echo "ERROR, Unknown board"
    exit 1
fi

echo "Waiting for DFU device..."
wait_for_dfu

dfu-util -i 0 -a 0 -D $BINARY -s 0x08000000:leave

