#/bin/sh

#BOARD=STM32F4DSCY
BOARD=RUUVIA

if [[ "$1" == "clean" ]]; then
	scons board=$BOARD -c
else
scons board=$BOARD romfs=compile prog
#scons board=$BOARD prog

fi
