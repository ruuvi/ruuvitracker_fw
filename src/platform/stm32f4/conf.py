# Configuration file for the STM32 microcontroller
import fnmatch
import glob
import os

# Place to keep ST micro's library
fwlibdir= 'src/platform/%s/FWLib' % platform

# Helper function to download library for us
# requires wget utility
def fetch_lib(dir, url):
	print "Downloading %s " % url, "to %s" % dir
	path = os.getcwd()
	os.chdir(dir)
	os.system("wget '"+url+"' -O fwlib.zip")
	os.system("unzip fwlib.zip")
	f = open(".downloaded","w")
	f.write("\n")
	f.close()
	os.chdir(path)

# Download FWlib if not exist
try:
	stat = os.stat(fwlibdir+"/.downloaded")
except OSError:
	fetch_lib(fwlibdir, "http://www.st.com/internet/com/SOFTWARE_RESOURCES/SW_COMPONENT/FIRMWARE/stm32_f105-07_f2_f4_usb-host-device_lib.zip")

# Specify different paths inside of ST's library
STDPeriphdir = fwlibdir+"/STM32_USB-Host-Device_Lib_V2.1.0/Libraries/STM32F4xx_StdPeriph_Driver"
CMSISdir = fwlibdir+"/STM32_USB-Host-Device_Lib_V2.1.0/Libraries/CMSIS"
USBdev = fwlibdir+"/STM32_USB-Host-Device_Lib_V2.1.0/Libraries/STM32_USB_Device_Library"
USBotg = fwlibdir+"/STM32_USB-Host-Device_Lib_V2.1.0/Libraries/STM32_USB_OTG_Driver"

# construct Include path
comp.Append(CPPPATH = [STDPeriphdir+"/inc" , CMSISdir+"/Include", CMSISdir+"/Device/ST/STM32F4xx/Include/", USBdev+"/Core/inc/", USBotg+"/inc/", USBdev+"/Class/cdc/inc/" ])

# Files to compile from ST's libraries
STD_files = "stm32f4xx_adc.c stm32f4xx_dac.c stm32f4xx_dma.c stm32f4xx_exti.c stm32f4xx_gpio.c stm32f4xx_i2c.c stm32f4xx_rcc.c " \
    + "stm32f4xx_usart.c stm32f4xx_spi.c stm32f4xx_tim.c misc.c"
USBotg_files = "usb_dcd.c usb_dcd_int.c usb_core.c"
USBcore_files = "usbd_req.c usbd_core.c usbd_ioreq.c"
USBclass_files =USBdev+"/Class/cdc/src/usbd_cdc_core.c"

# join lib files with correct paths
fwlib_files = " ".join( [ STDPeriphdir+"/src/%s" % ( f ) for f in STD_files.split() ] ) \
	+" "+ CMSISdir+"/Device/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s" \
	+" "+ " ".join( [ USBdev+"/Core/src/%s" % ( f ) for f in USBcore_files.split() ] ) \
	+" "+ " ".join( [ USBotg+"/src/%s" % ( f ) for f in USBotg_files.split() ] ) \
	+" "+ USBclass_files

# Files to compile from this folder
specific_files = "system_stm32f4xx.c platform.c platform_int.c uart.c platform_i2c.c platform_sha1.c sha1.c" \
    +" usb_bsp.c usbd_desc.c usbd_usr.c usbd_cdc_vcp.c stm32_usb.c" \
    +" lua_ruuvi.c"

ldscript = "stm32.ld"
  
# Prepend with path
specific_files = fwlib_files + " " + " ".join( [ "src/platform/%s/%s" % ( platform, f ) for f in specific_files.split() ] )
specific_files += " src/platform/cortex_utils.s src/platform/arm_cortex_interrupts.c"
ldscript = "src/platform/%s/%s" % ( platform, ldscript )

# Flags for compiler
comp.Append(CPPDEFINES = ["FOR" + cnorm( comp[ 'cpu' ] ),"FOR" + cnorm( comp[ 'board' ] ),'gcc'])
comp.Append(CPPDEFINES = [ 'USE_STDPERIPH_DRIVER', 'STM32F4XX', 'CORTEX_M4', 'HSE_VALUE=12000000', 'USE_EMBEDDED_PHY', 'USE_USB_OTG_FS'])

# Standard GCC Flags
comp.Append(CCFLAGS = ['-ffunction-sections','-fdata-sections','-fno-strict-aliasing','-Wall','-g'])

#'-nostartfiles',
comp.Append(LINKFLAGS = ['-nostartfiles', '-nostdlib','-T',ldscript,'-Wl,--gc-sections','-Wl,--allow-multiple-definition','-Wl,-Map=%s.map' % (output)])
comp.Append(ASFLAGS = ['-x','assembler-with-cpp','-c','-Wall','$_CPPDEFFLAGS'])
comp.Append(LIBS = ['c','gcc','m'])

#TARGET_FLAGS = ['-mthumb', '-mcpu=cortex-m4' ]  # Software Floating point
TARGET_FLAGS = ['-g', '-mthumb', '-mcpu=cortex-m4', '-mfloat-abi=hard', '-mfpu=fpv4-sp-d16'] # Hardware floating point. Requires supporting toolchain

# Configure General Flags for Target
comp.Prepend(CCFLAGS = [TARGET_FLAGS,'-mlittle-endian'])
comp.Prepend(LINKFLAGS = [TARGET_FLAGS,'-Wl,-e,Reset_Handler','-Wl,-static'])
comp.Prepend(ASFLAGS = TARGET_FLAGS)

# Toolset data
tools[ 'stm32f4' ] = {}

# Programming function
def progfunc_stm32( target, source, env ):
  outname = output + ".elf"
  os.system( "%s %s" % ( toolset[ 'size' ], outname ) )
  print "Generating binary image..."
  os.system( "%s -O binary %s %s.bin" % ( toolset[ 'bin' ], outname, output) )
  os.system( "%s -O ihex %s %s.hex" % ( toolset[ 'bin' ], outname, output ) )
  
tools[ 'stm32f4' ][ 'progfunc' ] = progfunc_stm32
