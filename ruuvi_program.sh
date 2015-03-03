#!/bin/bash
if [ -z "$BOARD" ]
then
    BOARD=RUUVITRACKER_C3
fi
FW_FILE=micropython/stmhal/build-$BOARD/firmware.dfu
if [ ! -f $FW_FILE ]
then
    FW_FILE=compiled-firmware/firmware.dfu
fi    


function wait_for_dfu() {
    while ! dfu-util -l |grep "Found DFU">/dev/null;do
	sleep 0.1
    done
}

echo "Waiting for DFU device..."
wait_for_dfu

# TODO: add udev rule to allow programming for the user
set -x
sudo dfu-util -a 0 -d 0483:df11 -D $FW_FILE
