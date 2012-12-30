# Hey Emacs, this is a -*- makefile -*-
#
# WinARM template makefile 
# by Martin Thomas, Kaiserslautern, Germany 
# <eversmith(at)heizung-thomas(dot)de>
#
# Released to the Public Domain
# Please read the make user manual!
#
# The user-configuration is based on the WinAVR makefile-template
# written by Eric B. Weddington, Jörg Wunsch, et al. but the internal
# handling used here is very different.
# This makefile can also be used with the GNU tools included in
# Yagarto, GNUARM or the Codesourcery packages. It should work
# on Unix/Linux-Systems too. Just a rather up-to-date GNU make is
# needed.
#
#
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Upload load-image to the device
#
# make filename.s = Just compile filename.c into the assembler code only
#
# make filename.o = Create object filename.o from filename.c (using CFLAGS)
#
# To rebuild project do "make clean" then "make all".
#
# Changes:
# - 17. Feb. 2005  - added thumb-interwork support (mth)
# - 28. Apr. 2005  - added C++ support (mth)
# - 29. Arp. 2005  - changed handling for lst-Filename (mth)
# -  1. Nov. 2005  - exception-vector placement options (mth)
# - 15. Nov. 2005  - added library-search-path (EXTRA_LIB...) (mth)
# -  2. Dec. 2005  - fixed ihex and binary file extensions (mth)
# - 22. Feb. 2006  - added AT91LIBNOWARN setting (mth)
# - 19. Apr. 2006  - option FLASH_TOOL (mth)
# - 23. Jun. 2006  - option USE_THUMB_MODE -> THUMB/THUMB_IW
# -  3. Aug. 2006  - added -ffunction-sections -fdata-sections to CFLAGS
#                    and --gc-sections to LDFLAGS. Only available for gcc >=4 (mth)
#                    (needs appropriate linker-script, remove them when using a
#                    "simple" linker-script)
# -  4. Aug. 2006  - pass SUBMDL-define to frontend (mth)
# - 11. Nov. 2006  - FLASH_TOOL-config, TCHAIN-config (mth)
# - 28. Mar. 2007  - remove .dep-Directory with rm -r -f and force "no error" (mth)
# - 24. Apr. 2007  - added "both" option for format (.bin and .hex) (mth)
# - 20. Aug. 2007  - extraincdirs in asflags, passing a "board"-define (mth)
# - 13. Sep. 2007  - create assembler from c-sources fixed (make foo.s for foo.c) (mth)
#                  - IMGEXT no longer used and removed (mth)
#                  - moved some entries (mth)
# - 25. Oct. 2007  - reverted 20070328-change (b/o "race condition" with 
#                    make clean all or when called from Eclipse) (mth)
#                  - removed "for flash" from objdump message-string (mth)
#                  - added same remarks (mth)
# - 30. Oct. 2007  - Support for an output-directory with all files 
#                    created during "make all". (mth)
#                  - modified targets which creates assembler (lower-case s) 
#                    from C-source using make <.c-file w/o ext.>.s (mth)
#                  - removed redundant/unused defines, overall cleanup (mth)
# - 10. Nov. 2007  - renamed TCHAIN to TCHAIN_PREFIX, other minor cleanups (mth)
# - 13. Mar. 2008  - renamed FORMAT to LOADFORMAT, edited some comments/messages (mth)
# - 13. Apr. 2009  - OpenOCD options for batch-programming (make program) (mth)
# -  1. May  2009  - replaced SUBMDL with CHIP (mth)
# - 15. Jul. 2009  - ComSpec environment-variable to select host-OS, should 
#                    increase compatibility (mth)
# -  1. Sep. 2009  - rename ROM_RUN->FLASH_RUN, VECT_TAB_ROM->VECT_TAB_FLASH (mth)
# - 11. Sep. 2009  - new target to create output directories. attempt for better
#                    "Win32 only" support (without "Unix"-shell and -tools) (mth)
#                    This is much faster on Win32 then MSYS/MinGW or Cygwin. (mth)
# -  5. Dec. 2009  - automatic selection of gcc or g++ for linking. g++ used when C++ 
#                    source-files are listed. -nostartfiles not used for C++ (mth)
# - 16. May. 2010  - on Win32 use set instead of env to get environment, removed
#                    flashing with uVision, added BOARD to CDEFS and ADEFS, phony
#                    targets listed in one line, removed setting for output-format
#                    and always create both (.hex and .bin), added debug-format gdb,
#                    rename AT91LIBNOWARN to DISABLESPECIALWARNINGS, removed target 
#                    finished, remark on rm/cs-rm, other minor clean-ups, tested
#                    with GNU make version 3.81 from CS and Cygwin (mth)
# - 13. Jun. 2010  - Trigger build when non-source files have changed 
#                    (see BUILDONCHANGE). (mth)
# - 22. Apr. 2012  - reminder: do not use -nostartfiles to avoid missing "_init"
#                    when calling __libc_init_array on startup
# -  2. Jun. 2012  - added option TCHAIN_ROOT so different toolchains with the
#                    same prefix can be selected
#                  - extra definition for linker-script to force re-link when
#                    linkerscript has been modified


# Toolchain path, set to empty to use the system's seach PATH
#TCHAIN_ROOT = $(ProgramFiles)/GNU Tools ARM Embedded/4.6 2012q1/bin/
#TCHAIN_ROOT = $(ProgramFiles)/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI/bin/
TCHAIN_ROOT = 
# Toolchain prefix (arm-none-eabi- -> arm-none-eabi-gcc.exe)
TCHAIN_PREFIX = $(TCHAIN_ROOT)arm-none-eabi-
#TCHAIN_PREFIX = arm-eabi-
#TCHAIN_PREFIX = arm-elf-
# cs-rm is a standard GNU rm which gets installed with CS G++ lite.
# Just the filename is different. Use REMOVE_CMD=rm in environments
# with rm (Linux, BSD, msys, Cygwin etc.)
#REMOVE_CMD=rm
REMOVE_CMD=cs-rm

# YES enables -mthumb option to flags for source-files listed 
# in SRC and CPPSRC and -mthumb-interwork option for all source
USE_THUMB_MODE = YES
#USE_THUMB_MODE = NO

# MCU name, submodel and board
# - MCU used for compiler-option (-mcpu)
# - SUBMDL used for linker-script name (-T) and passed as define
# - BOARD just passed as define (don't used '-' characters)
MCU      = cortex-m4
CHIP     = STM32F4xx_1024k
#BOARD    = STM32F4DISCOVERY
BOARD    = RUUVITRACKER_REVA

# *** This example only supports "FLASH_RUN" ***
# RUN_MODE is passed as define and used for the linker-script filename,
# the user has to implement the necessary operations for 
# the used mode (i.e. no copy of .data, remapping...)
# Create FLASH-Image
RUN_MODE=FLASH_RUN
# Create RAM-Image
#RUN_MODE=RAM_RUN

# Exception-vectors placement option is just passed as define,
# the user has to implement the necessary operations (i.e. remapping)
# Exception vectors in FLASH:
VECTOR_TABLE_LOCATION=VECT_TAB_FLASH
# Exception vectors in RAM:
#VECTOR_TABLE_LOCATION=VECT_TAB_RAM

# Directory for output files (lst, obj, dep, elf, sym, map, hex, bin etc.)
OUTDIR = $(RUN_MODE)

# Target file name (without extension).
TARGET = ruuvi

# Utility variables
APPLIBDIR      = ./Libraries
STMLIBDIR      = $(APPLIBDIR)
STMSPDDIR      = $(STMLIBDIR)/STM32F4xx_StdPeriph_Driver
STMSPDSRCDIR   = $(STMSPDDIR)/src
STMSPDINCDIR   = $(STMSPDDIR)/inc
CMSISCOREDIR   = $(STMLIBDIR)/CMSIS
CMSISCOREINCDIR= $(CMSISCOREDIR)/Include
CMSISDEVDIR    = $(CMSISCOREDIR)/ST/STM32F4xx
CMSISDEVINCDIR = $(CMSISDEVDIR)/Include
CMSISDEVSRCDIR = $(CMSISDEVDIR)/Source
#STMUTILDIR     = $(STMLIBDIR)/STM32F4-Discovery

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files
SRC = system_stm32f4xx.c stm32f4xx_it.c main.c hw_config.c

#SRC += $(STMUTILDIR)/stm32f4_discovery.c 
#SRC += $(STMUTILDIR)/stm32f4_discovery_lis302dl.c 
#SRC += $(STMUTILDIR)/stm32f4_discovery_audio_codec.c 

SRC += $(STMSPDSRCDIR)/misc.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_adc.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_dac.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_dma.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_exti.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_flash.c
SRC += $(STMSPDSRCDIR)/stm32f4xx_gpio.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_i2c.c
SRC += $(STMSPDSRCDIR)/stm32f4xx_rcc.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_spi.c
SRC += $(STMSPDSRCDIR)/stm32f4xx_syscfg.c
#SRC += $(STMSPDSRCDIR)/stm32f4xx_tim.c

#SRC += $(STMSPDSRCDIR)/stm32f4xx_usart.c

#SRC += $(STMLIBDIR)/STM32_USB_Device_Library/Core/src/usbd_core.c 
#SRC += $(STMLIBDIR)/STM32_USB_Device_Library/Core/src/usbd_ioreq.c 
#SRC += $(STMLIBDIR)/STM32_USB_Device_Library/Core/src/usbd_req.c 
#SRC += $(STMLIBDIR)/STM32_USB_Device_Library/Class/hid/src/usbd_hid_core.c
#SRC += $(STMLIBDIR)/STM32_USB_OTG_Driver/src/usb_dcd.c
#SRC += $(STMLIBDIR)/STM32_USB_OTG_Driver/src/usb_core.c
#SRC += $(STMLIBDIR)/STM32_USB_OTG_Driver/src/usb_dcd_int.c


# List C source files here which must be compiled in ARM-Mode (no -mthumb).
# use file-extension c for "c-only"-files
## just for testing, timer.c could be compiled in thumb-mode too
SRCARM = 

# List C++ source files here.
# use file-extension .cpp for C++-files (not .C)
CPPSRC = 

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension .cpp for C++-files (not .C)
#CPPSRCARM = $(TARGET).cpp
CPPSRCARM = 

# List Assembler source files here.
# Make them always end in a capital .S. Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = startup_stm32f4xx.S

# List Assembler source files here which must be assembled in ARM-Mode.
ASRCARM  = 

# Place project-specific -D (define) and/or -U options for C here.
CDEFS = -DUSE_STDPERIPH_DRIVER
CDEFS += -DSTM32F4XX
CDEFS += -DHSE_VALUE=12000000UL
#CDEFS += -DUSE_USB_OTG_FS
CDEFS += -DUSE_FULL_ASSERT
# enable modifications in STM's libraries
#CDEFS += -DMOD_MTHOMAS_STMLIB

# Place project-specific -D and/or -U options for 
# Assembler with preprocessor here.
ADEFS = 

# List any extra directories to look for include files here.
#    Each directory must be seperated by a space.
EXTRAINCDIRS  = $(STMSPDINCDIR) $(CMSISDEVINCDIR) $(CMSISCOREINCDIR)
#EXTRAINCDIRS += $(APPLIBDIR)/STM32_USB_Device_Library/Class/hid/inc
#EXTRAINCDIRS += $(APPLIBDIR)/STM32_USB_Device_Library/Core/inc
#EXTRAINCDIRS += $(APPLIBDIR)/STM32_USB_OTG_Driver/inc

# Extra libraries
#    Each library-name must be seperated by a space.
#    i.e. to link with libxyz.a, libabc.a and libefsl.a: 
#     EXTRA_LIBS = xyz abc efsl
#    for newlib-lpc (file: libnewlibc-lpc.a):
#     EXTRA_LIBS = newlib-lpc
EXTRA_LIBS =

# List non-source files which should trigger build here
#    Typically the Makefile and selected header-files
#    Entries must be seperated by a space.
BUILDONCHANGE = Makefile
#BUILDONCHANGE =

# Path to linker-scripts (see -T option)
LINKERSCRIPTPATH = .

# List any directories with files included from linker-scripts.
#    Each directory must be seperated by a space.
LINKERSCRIPTINC  = .

# List any extra directories to look for library files here.
#     Each directory must be seperated by a space.
EXTRA_LIBDIRS =

# Optimization level, can be [0, 1, 2, 3, s]. 
# 0 = Turn off optimization. Reduce compilation time and make debugging
#     produce the expected results.
# 1 = The compiler tries to reduce code size and execution time, without
#     performing any optimizations that take a great deal of compilation time.
# 2 = GCC performs nearly all supported optimizations that do not involve a 
#     space-speed tradeoff. As compared to -O1, this option increases
#     both compilation time and the performance of the generated code.
# 3 = Optimize yet more. Turns on -finline-functions and more.
# s = -Os enables all -O2 optimizations that do not typically increase code
#     size.
# (See gcc manual for further information)
OPT = s
#OPT = 2
#OPT = 3
#OPT = 0

# Debugging format.
#DEBUG = stabs
#DEBUG = dwarf-2
DEBUG = gdb

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Flash programming tool
FLASH_TOOL = OPENOCD
#FLASH_TOOL = ST-Link_CLI

# Some warnings can be disabled by this setting 
# (useful for the old single-file AT91-lib) 
#  yes - disable some warnings
#  no  - keep default settings
#DISABLESPECIALWARNINGS = yes
DISABLESPECIALWARNINGS = no

# ---------------------------------------------------------------------------
# Options for OpenOCD flash-programming
# see openocd.pdf/openocd.texi for further information
#
OOCD_LOADFILE+=$(OUTDIR)/$(TARGET).elf
# if OpenOCD is in the $PATH just set OPENOCDEXE=openocd
OOCD_EXE=./OpenOCD/bin/openocd
# debug level
OOCD_CL=-d0
#OOCD_CL=-d3
# interface and board/target settings (using the OOCD target-library here)
OOCD_CL+=-f board/stm32f4discovery.cfg
# initialize
OOCD_CL+=-c init
# if no SRST available:
## why unknown - it's documented... OOCD_CL+=-c "cortex_m3 reset_config sysresetreq"
# commands to prepare flash-write
OOCD_CL+= -c "reset halt"
# show the targets
OOCD_CL+=-c targets
# increase JTAG frequency a little bit - can be disabled for tests
OOCD_CL+= -c "adapter_khz 1000"
# disable polling (optional)
#OOCD_CL+= -c "poll off"
# flash-write and -verify
OOCD_CL+=-c "flash write_image erase $(OOCD_LOADFILE)" -c "verify_image $(OOCD_LOADFILE)"
# AIRCR SYSRESETREQ - workaround since sometimes the controller does not start after reset run
# but seems to "hang" in an NMI - should be removed once cortex_m3 reset_config works
OOCD_CL+=-c"mww 0xE000ED0C 0x05fa0004" -c "sleep 200"
# reset target
OOCD_CL+=-c "reset run"
# show the targets
OOCD_CL+=-c targets
# terminate OOCD after programming
OOCD_CL+=-c shutdown
# ---------------------------------------------------------------------------


ifdef VECTOR_TABLE_LOCATION
CDEFS += -D$(VECTOR_TABLE_LOCATION)
ADEFS += -D$(VECTOR_TABLE_LOCATION)
endif

CDEFS += -D$(RUN_MODE) -D$(CHIP) -D$(BOARD)
ADEFS += -D$(RUN_MODE) -D$(CHIP) -D$(BOARD)


# Compiler flags.

ifeq ($(USE_THUMB_MODE),YES)
THUMB    = -mthumb
THUMB_IW = -mthumb-interwork
else 
THUMB    = 
THUMB_IW = 
endif

#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-elf-gcc/arm-elf-g++)
CFLAGS =  -g$(DEBUG) 
CFLAGS += -O$(OPT)
CFLAGS += -mcpu=$(MCU) $(THUMB_IW) 
######################################################
# FPU Support. Make sure to enable the FPU at startup
# reminders: __ARM_PCS and __ARM_PCS_VFP -Wdouble-promotion
# float-abi softfp can be used with most/all toolchains. If no
#  fp-libraries are available software-emulation will be used
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -fno-strict-aliasing -Wdouble-promotion
# for float-abi hard the toolchain has to provide libraries which 
#  support this (CodeBench lite does not come with these libraries)  
#CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fno-strict-aliasing -Wdouble-promotion
######################################################
CFLAGS += $(CDEFS)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -I.
# when using ".ramfunc"s without attribute longcall:
#CFLAGS += -mlong-calls
# -mapcs-frame is important if gcc's interrupt attributes are used
# (at least from my eabi tests), not needed if assembler-wrappers are used 
CFLAGS += -mapcs-frame 
#CFLAGS += -fomit-frame-pointer
CFLAGS += -ffunction-sections -fdata-sections
#CFLAGS += -fstrict-volatile-bitfields
CFLAGS += -Wall -Wextra
#CFLAGS += -pedantic
CFLAGS += -Wcast-align -Wpointer-arith
CFLAGS += -Wredundant-decls -Wshadow -Wcast-qual -Wcast-align
CFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
# Compiler flags to generate dependency files:
#CFLAGS += -MD -MP -MF $(OUTDIR)/dep/$(@F).d 
CFLAGS += -MMD -MP -MF $(OUTDIR)/dep/$(@F).d
##testing:
#CFLAGS += -MF"$@" -MG -MM -MP -MT"$@" -MT"$(<:.c=.o)"
#CFLAGS += -fno-dwarf2-cfi-asm
#CFLAGS += -fno-strict-aliasing -Wno-nested-externs
#CFLAGS += -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables

# flags only for C
CONLYFLAGS += -Wnested-externs 
CONLYFLAGS += $(CSTANDARD)

ifeq ($(DISABLESPECIALWARNINGS),yes)
CFLAGS += -Wno-cast-qual
CONLYFLAGS += -Wno-missing-prototypes 
CONLYFLAGS += -Wno-strict-prototypes
CONLYFLAGS += -Wno-missing-declarations
endif

# flags only for C++ (arm-*-g++)
CPPFLAGS = -fno-rtti -fno-exceptions

# Assembler flags.
#  -Wa,...:    tell GCC to pass this to the assembler.
#  -ahlns:     create listing
#  -g$(DEBUG): have the assembler create line number information
ASFLAGS  = -mcpu=$(MCU) $(THUMB_IW) -I. -x assembler-with-cpp
ASFLAGS += -D__ASSEMBLY__ $(ADEFS)
ASFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
ASFLAGS += -Wa,-g$(DEBUG)
ASFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

# Link with the GNU C++ stdlib.
CPLUSPLUS_LIB = 
#CPLUSPLUS_LIB += -lsupc++

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -Wl,-Map=$(OUTDIR)/$(TARGET).map,--cref,--gc-sections
#LDFLAGS += -Wl,--start-group -lgcc -lc -lm -lnosys -Wl,--end-group
LDFLAGS += -Wl,--start-group -lgcc -lc -lm -Wl,--end-group
LDFLAGS += $(CPLUSPLUS_LIB)
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += $(patsubst %,-L%,$(LINKERSCRIPTINC))
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS)) 

# Set linker-script name depending on selected run-mode and chip
ifeq ($(RUN_MODE),RAM_RUN)
LDSCRIPT = $(LINKERSCRIPTPATH)/$(CHIP)_ram.ld
else
LDSCRIPT = $(LINKERSCRIPTPATH)/$(CHIP)_flash.ld 
endif
LDFLAGS +=-T$(LDSCRIPT)

# Autodetect environment
SHELL   = sh
ifneq ($(or $(COMSPEC), $(ComSpec)),)
$(info COMSPEC detected $(COMSPEC) $(ComSpec))
ifeq ($(findstring cygdrive,$(shell set)),)
SHELL:=$(or $(COMSPEC),$(ComSpec))
SHELL_IS_WIN32=1
else
$(info cygwin detected)
#override user-setting since cygwin has rm
REMOVE_CMD:=rm
endif
else
#most probaly a Unix/Linux/BSD system which should have rm
REMOVE_CMD:=rm
endif
$(info SHELL is $(SHELL), REMOVE_CMD is $(REMOVE_CMD))

# Define programs and commands.
CC      = "$(TCHAIN_PREFIX)gcc"
CPP     = "$(TCHAIN_PREFIX)g++"
AR      = "$(TCHAIN_PREFIX)ar"
OBJCOPY = "$(TCHAIN_PREFIX)objcopy"
OBJDUMP = "$(TCHAIN_PREFIX)objdump"
SIZE    = "$(TCHAIN_PREFIX)size"
NM      = "$(TCHAIN_PREFIX)nm"
REMOVE  = "$(REMOVE_CMD)" -f

# Define Messages
# English
MSG_BEGIN = --------  begin, mode: $(RUN_MODE)  --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before:
MSG_SIZE_AFTER = Size after build:
MSG_LOAD_FILE = Creating load file:
MSG_EXTENDED_LISTING = Creating Extended Listing/Disassembly:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = ---- Linking:
MSG_COMPILING = ---- Compiling C:
MSG_COMPILING_ARM = ---- Compiling C ARM-only:
MSG_COMPILINGCPP = ---- Compiling C++:
MSG_COMPILINGCPP_ARM = ---- Compiling C++ ARM-only:
MSG_ASSEMBLING = ---- Assembling:
MSG_ASSEMBLING_ARM = ---- Assembling ARM-only:
MSG_CLEANING = Cleaning project:
MSG_ASMFROMC = "Creating asm-File from C-Source:"
MSG_ASMFROMC_ARM = "Creating asm-File from C-Source (ARM-only):"
MSG_BOOTLOADER_REMINDER = "You may have to bring the target in bootloader-mode manually"

# List of all source files.
ALLSRC     = $(ASRCARM) $(ASRC) $(SRCARM) $(SRC) $(CPPSRCARM) $(CPPSRC)
# List of all source files without directory and file-extension.
ALLSRCBASE = $(notdir $(basename $(ALLSRC)))

# Define all object files.
ALLOBJ     = $(addprefix $(OUTDIR)/, $(addsuffix .o, $(ALLSRCBASE)))

# Define all listing files (used for make clean).
LSTFILES   = $(addprefix $(OUTDIR)/, $(addsuffix .lst, $(ALLSRCBASE)))
# Define all depedency-files (used for make clean).
DEPFILES   = $(addprefix $(OUTDIR)/dep/, $(addsuffix .o.d, $(ALLSRCBASE)))

elf: $(OUTDIR)/$(TARGET).elf
lss: $(OUTDIR)/$(TARGET).lss 
sym: $(OUTDIR)/$(TARGET).sym
hex: $(OUTDIR)/$(TARGET).hex
bin: $(OUTDIR)/$(TARGET).bin

# Default target.
all: begin createdirs gccversion build sizeafter end

# Target for the build-sequence.
build: elf lss sym hex bin

# Create output directories.
ifdef SHELL_IS_WIN32
createdirs:
	-@md $(OUTDIR) >NUL 2>&1 || echo "" >NUL
	-@md $(OUTDIR)\dep >NUL 2>&1 || echo "" >NUL
else
createdirs:
	-@mkdir $(OUTDIR) 2>/dev/null || echo "" >/dev/null
	-@mkdir $(OUTDIR)/dep 2>/dev/null || echo "" >/dev/null
endif

# Eye candy.
begin:
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)

# Display sizes of sections.
ELFSIZE = $(SIZE) -A  $(OUTDIR)/$(TARGET).elf
sizebefore:
#	@if [ -f  $(OUTDIR)/$(TARGET).elf ]; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); echo; fi

sizeafter:
#	@if [ -f  $(OUTDIR)/$(TARGET).elf ]; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); echo; fi
	@echo $(MSG_SIZE_AFTER)
	$(ELFSIZE)

# Display compiler version information.
gccversion : 
	@$(CC) --version
	@$(CC) -print-multi-lib

# Program the device.
ifeq ($(FLASH_TOOL),OPENOCD)
# Program the device with Dominic Rath's OPENOCD in "batch-mode"
program: $(OUTDIR)/$(TARGET).elf
	@echo "Programming with OPENOCD"
ifdef SHELL_IS_WIN32 
	$(subst /,\,$(OOCD_EXE)) $(OOCD_CL)
else
	$(OOCD_EXE) $(OOCD_CL)
endif
else
# Program the device using ST-LINK_CLI through the embedded ST-LINK V2
program: $(OUTDIR)/$(TARGET).hex
    @echo $(MSG_BOOTLOADER_REMINDER)
	-ST-LINK_CLI/ST-LINK_CLI -c SWD -P $(OUTDIR)/$(TARGET).hex -V -Rst -Run
endif

# Create final output file in ihex format from ELF output file (.hex).
%.hex: %.elf
	@echo $(MSG_LOAD_FILE) $@
	$(OBJCOPY) -O ihex $< $@
	
# Create final output file in raw binary format from ELF output file (.bin)
%.bin: %.elf
	@echo $(MSG_LOAD_FILE) $@
	$(OBJCOPY) -O binary $< $@

# Create extended listing file/disassambly from ELF output file.
# using objdump (testing: option -C)
%.lss: %.elf
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -S -C -r $< >$@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< >$@

# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(ALLOBJ)
%.elf:  $(ALLOBJ) $(BUILDONCHANGE) $(LDSCRIPT)
	@echo $(MSG_LINKING) $@
# use $(CC) for C-only projects or $(CPP) for C++-projects:
ifeq "$(strip $(CPPSRC)$(CPPARM))" ""
	$(CC) $(THUMB) $(CFLAGS) $(ALLOBJ) --output $@ -nostartfiles $(LDFLAGS)
else
	$(CPP) $(THUMB) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
endif


# Assemble: create object files from assembler source files.
define ASSEMBLE_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_ASSEMBLING) $$< to $$@
	$(CC) -c $(THUMB) $$(ASFLAGS) $$< -o $$@ 
endef
$(foreach src, $(ASRC), $(eval $(call ASSEMBLE_TEMPLATE, $(src)))) 

# Assemble: create object files from assembler source files. ARM-only
define ASSEMBLE_ARM_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_ASSEMBLING_ARM) $$< to $$@
	$(CC) -c $$(ASFLAGS) $$< -o $$@ 
endef
$(foreach src, $(ASRCARM), $(eval $(call ASSEMBLE_ARM_TEMPLATE, $(src)))) 


# Compile: create object files from C source files.
define COMPILE_C_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_COMPILING) $$< to $$@
	$(CC) -c $(THUMB) $$(CFLAGS) $$(CONLYFLAGS) $$< -o $$@ 
endef
$(foreach src, $(SRC), $(eval $(call COMPILE_C_TEMPLATE, $(src)))) 

# Compile: create object files from C source files. ARM-only
define COMPILE_C_ARM_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_COMPILING_ARM) $$< to $$@
	$(CC) -c $$(CFLAGS) $$(CONLYFLAGS) $$< -o $$@ 
endef
$(foreach src, $(SRCARM), $(eval $(call COMPILE_C_ARM_TEMPLATE, $(src)))) 


# Compile: create object files from C++ source files.
define COMPILE_CPP_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_COMPILINGCPP) $$< to $$@
	$(CC) -c $(THUMB) $$(CFLAGS) $$(CPPFLAGS) $$< -o $$@ 
endef
$(foreach src, $(CPPSRC), $(eval $(call COMPILE_CPP_TEMPLATE, $(src)))) 

# Compile: create object files from C++ source files. ARM-only
define COMPILE_CPP_ARM_TEMPLATE
$(OUTDIR)/$(notdir $(basename $(1))).o : $(1) $(BUILDONCHANGE)
	@echo $(MSG_COMPILINGCPP_ARM) $$< to $$@
	$(CC) -c $$(CFLAGS) $$(CPPFLAGS) $$< -o $$@ 
endef
$(foreach src, $(CPPSRCARM), $(eval $(call COMPILE_CPP_ARM_TEMPLATE, $(src)))) 


# Compile: create assembler files from C source files. ARM/Thumb
$(SRC:.c=.s) : %.s : %.c $(BUILDONCHANGE)
	@echo $(MSG_ASMFROMC) $< to $@
	$(CC) $(THUMB) -S $(CFLAGS) $(CONLYFLAGS) $< -o $@

# Compile: create assembler files from C source files. ARM only
$(SRCARM:.c=.s) : %.s : %.c $(BUILDONCHANGE)
	@echo $(MSG_ASMFROMC_ARM) $< to $@
	$(CC) -S $(CFLAGS) $(CONLYFLAGS) $< -o $@

# Target: clean project.
clean: begin clean_list end

clean_list :
	@echo $(MSG_CLEANING)
	$(REMOVE) $(OUTDIR)/$(TARGET).map
	$(REMOVE) $(OUTDIR)/$(TARGET).elf
	$(REMOVE) $(OUTDIR)/$(TARGET).hex
	$(REMOVE) $(OUTDIR)/$(TARGET).bin
	$(REMOVE) $(OUTDIR)/$(TARGET).sym
	$(REMOVE) $(OUTDIR)/$(TARGET).lss
	$(REMOVE) $(ALLOBJ)
	$(REMOVE) $(LSTFILES)
	$(REMOVE) $(DEPFILES)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRCARM:.c=.s)
	$(REMOVE) $(CPPSRC:.cpp=.s)
	$(REMOVE) $(CPPSRCARM:.cpp=.s)

# Include the dependency files.
##-include $(wildcard dep/*)
-include $(wildcard *.d)

# Listing of phony targets.
.PHONY : all begin end sizebefore sizeafter gccversion build elf hex bin lss sym clean clean_list program createdirs

