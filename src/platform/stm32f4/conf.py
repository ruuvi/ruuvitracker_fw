# Configuration file for the STM32 microcontroller
import fnmatch
import glob
import os

fwlibdir= 'src/platform/%s/FWLib' % platform

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

#Download FWlib if not exist
try:
	stat = os.stat(fwlibdir+"/.downloaded")
except OSError:
	fetch_lib(fwlibdir, "http://www.st.com/internet/com/SOFTWARE_RESOURCES/SW_COMPONENT/FIRMWARE/stm32f4_dsp_stdperiph_lib.zip")

STDPeriphdir = fwlibdir+"/STM32F4xx_DSP_StdPeriph_Lib_V1.0.1/Libraries/STM32F4xx_StdPeriph_Driver"
CMSISdir = fwlibdir+"/STM32F4xx_DSP_StdPeriph_Lib_V1.0.1/Libraries/CMSIS"

comp.Append(CPPPATH = [STDPeriphdir+"/inc" , CMSISdir+"/Include", CMSISdir+"/Device/ST/STM32F4xx/Include/" ])

fwlib_files = " ".join(glob.glob(STDPeriphdir+"/src/*.c")) \
	+" "+ CMSISdir+"/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c" \
	+" "+ CMSISdir+"/Device/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s"
#print "FWLib: %s " % fwlib_files 

specific_files = "platform.c platform_int.c uart.c platform_i2c.c"

#ldscript = "stm32f4xx_flash.ld" 
ldscript = "stm32.ld"
  
# Prepend with path
specific_files = fwlib_files + " " + " ".join( [ "src/platform/%s/%s" % ( platform, f ) for f in specific_files.split() ] )
specific_files += " src/platform/cortex_utils.s src/platform/arm_cortex_interrupts.c"
ldscript = "src/platform/%s/%s" % ( platform, ldscript )

comp.Append(CPPDEFINES = ["FOR" + cnorm( comp[ 'cpu' ] ),"FOR" + cnorm( comp[ 'board' ] ),'gcc'])
comp.Append(CPPDEFINES = [ 'USE_STDPERIPH_DRIVER', 'STM32F4XX', 'CORTEX_M4'])

# Standard GCC Flags
comp.Append(CCFLAGS = ['-ffunction-sections','-fdata-sections','-fno-strict-aliasing','-Wall','-g'])

#'-nostartfiles',
comp.Append(LINKFLAGS = ['-nostartfiles', '-nostdlib','-T',ldscript,'-Wl,--gc-sections','-Wl,--allow-multiple-definition','-Wl,-Map=%s.map' % (output)])
comp.Append(ASFLAGS = ['-x','assembler-with-cpp','-c','-Wall','$_CPPDEFFLAGS'])
comp.Append(LIBS = ['c','gcc','m'])

TARGET_FLAGS = ['-mcpu=cortex-m4', '-mthumb']

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
