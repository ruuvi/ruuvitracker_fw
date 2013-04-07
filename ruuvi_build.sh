#!/bin/bash
# A simple script used for building eLua for the RuuviTracker project
# Author: Seppo Takalo

source ./ruuvi.conf

print_usage() {
    echo "RuuviTracker build script. (wrapper for SCons)"
    echo "usge: $0 [-h] [clean]"
    echo "      -h :  Print this help"
    echo "      clean: clean all instead of build."
}


#Build cross-compiler first
if [ ! -f luac.cross.elf ]; then
    scons -f cross-lua.py
fi

case $1 in
    clean)
	scons board=$BOARD -c
	;;
    "-h")
	print_usage
	;;
    *)
	scons board=$BOARD $COMPILE prog
	;;
esac

