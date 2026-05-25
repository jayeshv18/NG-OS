ENTRY(kernel_main)
SECTIONS
{
    . = 0x9000;

    .text : ALIGN(4)
    {
        build/stage2.o(.text)
        *(.text)
    }

    .rodata : ALIGN(4) { *(.rodata) }
    .data : ALIGN(4) { *(.data) }
    .bss : ALIGN(4) {
        *(COMMON)
        *(.bss)
    }
}