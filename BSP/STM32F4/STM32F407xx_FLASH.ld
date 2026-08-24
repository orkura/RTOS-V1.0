/*
 * STM32F407ZGT6 linker layout for the RT-Thread Nano BSP.
 *
 * Address allocation has two layers:
 *   1. MEMORY describes the physical memory regions provided by the MCU.
 *   2. SECTIONS maps compiler-generated input sections into those regions.
 *
 * For initialized .data placed in RAM, the virtual memory address (VMA) is
 * in RAM while the load memory address (LMA) is in FLASH. Reset_Handler copies
 * the initial values from the LMA to the VMA and clears .bss before entering
 * RT-Thread.
 */

/* Execution starts at Reset_Handler in the startup assembly after reset. */
ENTRY(Reset_Handler)

/*
 * STM32F407ZGT6 on-chip memory:
 *   RAM    - 128 KiB main SRAM accessible by the CPU, DMA, and bus masters.
 *   CCMRAM - 64 KiB core-coupled data RAM for fast CPU-only access.
 *   FLASH  - 1 MiB on-chip program Flash.
 *
 * CCMRAM and main SRAM are not contiguous and cannot be combined into one
 * 192 KiB memory region.
 */
MEMORY
{
    RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
}

/*
 * The initial MSP is at the top of main SRAM. The Cortex-M4 MSP grows toward
 * lower addresses and is primarily used during reset startup and exception or
 * interrupt handling. Each RT-Thread thread has a separate thread stack.
 *
 * _Min_Heap_Size is the minimum system heap capacity required at link time.
 * _Min_Stack_Size is the address-space budget reserved for the MSP at the top
 * of main SRAM.
 */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x2000;

SECTIONS
{
    /*
     * The Cortex-M4 interrupt vector table must be placed at the beginning of
     * the firmware image. KEEP prevents --gc-sections from discarding the table,
     * which is not referenced through an ordinary function call.
     */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Executable code, compiler helpers, and C runtime init/fini entry points. */
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

    /* Const objects, string literals, and other data read directly from Flash. */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /*
     * RT-Thread INIT_*_EXPORT() automatic initialization table.
     * Numeric suffixes identify initialization stages and must remain ordered.
     * KEEP prevents --gc-sections from discarding functions or descriptors that
     * are referenced only indirectly through this table.
     */
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

    /* Aliases let RT_DEBUG_INIT and non-debug builds share the same layout. */
    PROVIDE(__rt_init_desc_rti_start = __rt_init_rti_start);
    PROVIDE(__rt_init_desc_rti_board_start = __rt_init_rti_board_start);
    PROVIDE(__rt_init_desc_rti_board_end = __rt_init_rti_board_end);
    PROVIDE(__rt_init_desc_rti_end = __rt_init_rti_end);

    /* FinSH/MSH command table, enumerated between the boundary symbols. */
    FSymTab :
    {
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
    } > FLASH

    /* ARM exception unwind data retained for toolchain runtime support. */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    /* ARM exception index table and its boundaries for the runtime. */
    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    /*
     * GCC/C runtime function arrays. Preinit and init entries run before the
     * application; fini entries belong to program termination, which embedded
     * systems normally do not reach.
     */
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

    /* Start of the .data initial-value image in Flash for Reset_Handler. */
    _sidata = LOADADDR(.data);

    /*
     * Initialized global/static variables and RamFunc code that executes in RAM.
     * "> RAM AT > FLASH" places the runtime address in RAM and the initial image
     * in Flash.
     */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /*
     * CPU-only data explicitly assigned to .ccmram.
     * NOLOAD means that the firmware contains no initial image. The current
     * Reset_Handler neither copies nor clears this section, so users must
     * initialize it at runtime. CCMRAM must not be used for DMA buffers.
     */
    .ccmram (NOLOAD) :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } > CCMRAM

    /*
     * Uninitialized or zero-initialized global/static variables. This section
     * consumes RAM but no data payload in the firmware image. Reset_Handler
     * clears the entire [_sbss, _ebss) range during startup.
     */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /*
     * Automatic system heap layout: the heap starts at the first 8-byte-aligned
     * address after .bss and ends at the lower boundary of the MSP reserve.
     * rt_hw_board_init() passes these symbols to rt_system_heap_init(), so the
     * RT-Thread heap capacity automatically follows changes in static data size.
     *
     * __msp_stack_limit__ describes a policy boundary only. Cortex-M4 hardware
     * does not automatically stop the MSP from crossing it. Validate the stack
     * budget with high-water-mark measurements or MPU protection when required.
     */
    __heap_start__ = ALIGN(_ebss, 8);
    __heap_end__ = _estack - _Min_Stack_Size;
    __msp_stack_limit__ = __heap_end__;

    /* C runtimes such as newlib may reference end/_end after static data. */
    PROVIDE(end = __heap_start__);
    PROVIDE(_end = __heap_start__);

    /* Fail at link time if static data leaves too little room for heap and MSP. */
    ASSERT(__heap_end__ >= (__heap_start__ + _Min_Heap_Size),
           "RAM is not enough for the RT-Thread heap and MSP stack")
}
