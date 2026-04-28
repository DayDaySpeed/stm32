ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  _sidata = _etext;

  .data : AT(_sidata)
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + 0x400;
    . = . + 0x400;
    . = ALIGN(8);
  } > RAM
}
