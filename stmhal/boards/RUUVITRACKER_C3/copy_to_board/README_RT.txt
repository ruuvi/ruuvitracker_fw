# This is RuuviTracker board, running MicroPython (http://micropython.org/)

Should you need to flash a newer firmware it can be obtained from https://github.com/RuuviTracker/ruuvitracker_fw/tree/MicroPython/compiled-firmware

Linux/Mac instructions for dfu-util: https://github.com/micropython/micropython/wiki/Pyboard-Firmware-Update#flashing
Windows: http://micropython.org/resources/Micro-Python-Windows-setup.pdf

Bootloader can be activated by removing battery, then holding down the bootloader button while reconnecting (or calling pyb.bootloader() from the REPL)


## We include some useful python modules as "drivers"

The official (and sometimes outdated) source for these files is https://github.com/RuuviTracker/ruuvitracker_fw/tree/MicroPython/stmhal/boards/RUUVITRACKER_C3/copy_to_board

Bleeding edge code at https://github.com/rambo/ruuvitracker_fw/tree/MicroPython_rambo/stmhal/boards/RUUVITRACKER_C3/copy_to_board

There are no threads in micropython if (when) you need concurrency you need to use coroutines, read https://github.com/RuuviTracker/ruuvitracker_fw/blob/MicroPython/Best-Practises.md
