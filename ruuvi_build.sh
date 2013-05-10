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

build() {
    # Build cross compiler if not found
    if [ ! -f luac.cross.elf ]; then
	scons -f cross-lua.py
    fi
    # If TAGS file found, update tags&cscope
    if [ -f TAGS ]; then
	./generate_tags.sh >/dev/null 2>&1 &  # Run silently on background
    fi
    # Build
    scons --jobs 4 board=$BOARD $COMPILE prog
}

clean() {
    scons --jobs 4 board=$BOARD -c
    rm elua_lua_*.{bin,elf,hex,map} >/dev/null 2>&1
}

case $1 in
    clean)
	clean
	;;
    "-h")
	print_usage
	;;
    *)
	build
	;;
esac

