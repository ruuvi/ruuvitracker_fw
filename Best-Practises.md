# Best practises for developing on RuuviTracker with MicroPython

## Reloading modules

Re-importing will not (usually) work, best way is to use ctrl-D in the REPL to
do a soft reboot, this keeps all connections (terminal and flash-drive) intact
but clears Python memory. **NOTE**: this does *not* reset GPIOs either so
if your module initializations make assumptions about GPIO status' after reset
you will be in a world of pain.
