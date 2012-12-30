/*
*****************************************************************************
**
**  File        : stm32_flash.ld
**
**  Abstract    : Linker script for STM32F207IG Device with
**                1024KByte FLASH, 112KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed “as is,” without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Atollic AB permit registered Atollic TrueSTUDIO(R) users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the Atollic TrueSTUDIO(R) toolchain.
**
**
**  Not so minor modifications by Martin Thomas (not Attolic) inspired
**  by the default linker-script (arm-none-eabi-ld --verbose), the CS3 linker-
**  scripts, binutils-documentations, GNU Tools for ARM Embedded Processors
**  readme-file and examples for other targets.
**  No extra copyright on modifications.
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 112K
  MEMORY_B1 (rx)  : ORIGIN = 0x60000000, LENGTH = 0K
}

/* Highest address of the user mode stack */
/* _estack = 0x2001c000; */    /* end of 112K RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x3000; /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */


/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    /* mthomas: moved isr_vector into .text output-section so it shows up in the disassamble without additional objdump options */
    /* KEEP(*(.isr_vector))*/ /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    CREATE_OBJECT_SYMBOLS
    . = ALIGN(4);
    ___SVECTOR = .;
    KEEP(*(.isr_vector)) /* Startup code */
    ASSERT (. != ___SVECTOR, "No isr_vector input-section!");
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text.*)         /* .text* sections (code) */
    *(.gnu.linkonce.t.*)
    *(.plt)
    *(.gnu.warning)
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.vfp11_veneer)

    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata.*)       /* .rodata* sections (constants, strings, etc.) */
    *(.gnu.linkonce.r.*)

    *(.gcc_except_table)
    KEEP(*(.eh_frame_hdr*))
    KEEP(*(.eh_frame*))
    
    . = ALIGN(4);
    KEEP (*(.init))

    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;

    . = ALIGN(4);
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;

    . = ALIGN(4);
    KEEP (*(.fini))

    . = ALIGN(4);
    __fini_array_start = .;
    KEEP (*(.fini_array))
    KEEP (*(SORT(.fini_array.*)))
    __fini_array_end = .;
    
    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)
 		
  } >FLASH

  .ARM.extab : 
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > FLASH
  __exidx_end = .;
  
  .text.align_etext :
  {
    . = ALIGN(8);
    _etext = .;
  } >FLASH


  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(vtable)
    *(.got.plt) *(.got)
    *(.shdata)
    *(.data)           /* .data sections */
    *(.data.*)          /* .data* sections */
    *(.gnu.linkonce.d.*)

    . = ALIGN(4);
    *(.ram_function*)

    . = ALIGN(8);
    _edata = .;        /* define a global symbol at data end */
  } >RAM

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)

    . = ALIGN(8);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack (NOLOAD):
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM

  /* MEMORY_bank1 section, code must be located here explicitly            */
  /* Example: extern int foo(void) __attribute__ ((section (".mb1text"))); */
  .memory_b1_text :
  {
    *(.mb1text)        /* .mb1text sections (code) */
    *(.mb1text*)       /* .mb1text* sections (code)  */
    *(.mb1rodata)      /* read-only data (constants) */
    *(.mb1rodata*)
  } >MEMORY_B1
  
  .stab 0 (NOLOAD) : { *(.stab) }
  .stabstr 0 (NOLOAD) : { *(.stabstr) }
  /* DWARF debug sections.
   * Symbols in the DWARF debugging sections are relative to the beginning
   * of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) KEEP (*(.gnu.attributes)) }
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }

  /* Remove information from the standard libraries */
  /*
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
    *(.note.GNU-stack)
  }
  */
}
