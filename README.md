# RuuviTracker with [MicroPython][upy]

## Using

See [MicroPython documentation](http://docs.micropython.org/en/latest/quickref.html) for details about execution environment.

TODO: Document the board specific python modules as they get more functionality.

### REPL quickstart

On Linux the REPL is on ACM device, `/dev/ttyACM0` if you don't have any other CDC serial ports.

On OSX it's on `/dev/tty.usbmodemXXXX` (exact number is probably board specific).

## Flashing == Installing

Run `ruuvi_program.sh`, this will look for compiled firmware and install the "best", you need sudo rights and df-util.

Disconnect USB and battery to do a full hw-reset and reconnect USB to get the serial terminal and board flash-drive.

Copy the files & directories under `stmhal/boards/RUUVITRACKER_C3/copy_to_board` to the board flash-drive.


## Building

NOTE: Building requires a proper git clone, we use submodules and you must be able to init/update them too.

Building is simple, run `./ruuvi_build.sh`

If you want to build for a different board than RUUVITRACKER_C3 (which is revC2 compatible if you happen to have C2) run `BOARD=MYBOARD ./ruuvi_build.sh`,
in this case you must have your board files in the standard location under `stmhal/boards/`.


[upy]: http://micropython.org/