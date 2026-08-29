/* STM32H743VIT6 memory layout for the RT-Thread Nano BSP. */

ENTRY(Reset_Handler)

MEMORY
{
    DTCMRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    RAM     (xrw) : ORIGIN = 0x24000000, LENGTH = 512K
    RAM_D2  (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
    RAM_D3  (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
    ITCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
}

/*
 * Keep the first BSP revision aligned with the CubeMX-generated layout:
 * initialized data, zero-initialized data, the RT-Thread heap, and the MSP
 * reserve all live in DTCM. Other RAM domains remain available for future
 * explicitly placed sections.
 */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x2000;

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
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .rti_fn :
    {
        . = ALIGN(4);
        __rt_init_rti_start = .;
        KEEP(*(.rti_fn.0))

        __rt_init_rti_board_start = .;
        KEEP(*(.rti_fn.0.end))
        KEEP(*(.rti_fn.1))

        __rt_init_rti_board_end = .;
        KEEP(*(.rti_fn.1.end))
        KEEP(*(.rti_fn.2))
        KEEP(*(.rti_fn.3))
        KEEP(*(.rti_fn.4))
        KEEP(*(.rti_fn.5))
        KEEP(*(.rti_fn.6))

        __rt_init_rti_end = .;
        KEEP(*(.rti_fn.6.end))
        . = ALIGN(4);
    } > FLASH

    PROVIDE(__rt_init_desc_rti_start = __rt_init_rti_start);
    PROVIDE(__rt_init_desc_rti_board_start = __rt_init_rti_board_start);
    PROVIDE(__rt_init_desc_rti_board_end = __rt_init_rti_board_end);
    PROVIDE(__rt_init_desc_rti_end = __rt_init_rti_end);

    FSymTab :
    {
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        __data_start__ = _sdata;
        *(.data)
        *(.data*)
        *(.tdata)
        *(.tdata*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
        __data_end__ = _edata;
    } > DTCMRAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(.tbss)
        *(.tbss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCMRAM

    __heap_start__ = ALIGN(_ebss, 8);
    __heap_end__ = _estack - _Min_Stack_Size;
    __msp_stack_limit__ = __heap_end__;

    PROVIDE(end = __heap_start__);
    PROVIDE(_end = __heap_start__);

    ASSERT(__heap_end__ >= (__heap_start__ + _Min_Heap_Size),
           "DTCMRAM is not enough for the RT-Thread heap and MSP stack")
}
