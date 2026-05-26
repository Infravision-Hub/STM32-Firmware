ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
  RAM    (xrw)  : ORIGIN = 0x20000000, LENGTH = 30K
  FLASH  (rx)   : ORIGIN = 0x08000000, LENGTH = 256K
}

SECTIONS
{
  .isr_vector 0x08000000 :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    KEEP(*(.isr_vector*))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    . = . + 0x400;
    . = . + 0x400;
    . = ALIGN(8);
  } >RAM
}