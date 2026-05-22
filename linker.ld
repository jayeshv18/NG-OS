/* Tell the linker that kernel_main is our entry point */
ENTRY(kernel_main)

SECTIONS
{
    /* We want our kernel to live at the very top of virtual memory,
       specifically starting at 0xffffffff80000000 plus 2 Megabytes */
    . = 0xffffffff80200000;

    /* The .text section (Code) */
    .text : {
        *(.text)
    }

    /* Move to the next 4KB (0x1000) boundary before starting .rodata */
    . = ALIGN(0x1000);

    /* The .rodata section (Read-Only Data, like string literals) */
    .rodata : {
        *(.rodata)
    }

    /* Move to the next 4KB boundary before starting .data */
    . = ALIGN(0x1000);

    /* The .data section (Initialized variables) */
    .data : {
        *(.data)
    }

    /* Move to the next 4KB boundary before starting .bss */
    . = ALIGN(0x1000);

    /* The .bss section (Uninitialized variables) */
    .bss : {
        *(COMMON)
        *(.bss)
    }
}