# Best practises for developing on RuuviTracker with MicroPython

## Coroutines & generators FTW

Read the following

  - <http://www.dabeaz.com/generators/>
  - <http://www.dabeaz.com/coroutines/> (this especially will help to understand uasyncio)
  - <http://www.dabeaz.com/finalgenerator/>

Many list-comprehensions can be written as generator expressions, this saves memory (and increases performance).

Some OOP patterns can be reworked to be coroutines, this may increase performance (and save memory).

## Preallocate memory for exceptions in interrupt handlers

This will make debugging easier, add to `boot.py`

    import micropython
    micropython.alloc_emergency_exception_buf(100)

See [MicroPython documentation][emergbuf] for more info.

[emergbuf]: http://micropython.readthedocs.org/en/latest/library/micropython.html#micropython.alloc_emergency_exception_buf

## Reloading modules

Re-importing will not (usually) work, best way is to use ctrl-D in the REPL to
do a soft reboot, this keeps all connections (terminal and flash-drive) intact
but clears Python memory. **NOTE**: this does *not* reset GPIOs either so
if your module initializations make assumptions about GPIO status' after reset
you will be in a world of pain.
