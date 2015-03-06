#!/bin/bash
if [ -z "$BOARD" ]
then
    BOARD=RUUVITRACKER_C3
fi
git submodule init
git submodule update  --remote
if [ ! -d micropython/stmhal/boards/$BOARD ]
then
    ln -s ../../../stmhal/boards/$BOARD micropython/stmhal/boards/$BOARD
fi    
cd micropython/stmhal/
BOARD=$BOARD make
