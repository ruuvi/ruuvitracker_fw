#!/bin/bash

src_files() {
	find ./inc ./src -type f -name \*.h -or -name \*.c
}


src_files | xargs etags

src_files > cscope.files
cscope -b -k

