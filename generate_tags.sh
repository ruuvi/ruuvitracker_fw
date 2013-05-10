#!/bin/bash

src_files() {
	find ./inc ./src -maxdepth 3 -type f -name \*.h -or -name \*.c
}


src_files | xargs etags

src_files > cscope.files
cscope -b -q -k

