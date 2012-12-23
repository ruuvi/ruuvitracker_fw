#/bin/sh

if [[ "$1" == "clean" ]]; then
	scons board=STM32F4DSCY -c
else
scons board=STM32F4DSCY prog

fi
