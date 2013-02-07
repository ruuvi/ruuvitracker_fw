#/bin/sh
# A simple script used for building eLua for the RuuviTracker project
# Author: Seppo Takalo

source ./ruuvi.conf

if [[ "$1" == "clean" ]]; then
	scons board=$BOARD -c
else
scons board=$BOARD $COMPILE prog

fi
