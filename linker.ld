/*
 1. THE ENTRY POINT
 * We no longer enter at 'kernel_main'. We must enter at the assembly label
 * '_start' defined in boot.asm so we can catch GRUB's memory map pointer.
 */
ENTRY(_start)

SECTIONS
{
    /*
     2. THE LOAD ADDRESS (1 Megabyte)
     * In Phase 1, we told the BIOS to load us at 0x9000.
     * The Multiboot standard dictates that GRUB will load our kernel exactly
     * at the 1 Megabyte mark in physical RAM (0x100000).
     * We command the Linker to calculate all memory pointers based on this starting line.
     */
    . = 1M;

    /*
     3. THE TEXT SECTION (Executable Code)
     * We align sections to 4 Kilobyte boundaries (ALIGN(4K)) because 4KB is the
     * exact size of a physical memory block. This is prep work for our Paging system.
     */
    .text : ALIGN(4K)
    {
        /* CRITICAL: The Multiboot header MUST be the absolute first thing in the file.
         * If it gets buried under C code, GRUB's 8KB scanner will miss it and refuse to boot.
         */
        build/boot.o(.multiboot)

        /* After the header, dump the rest of the boot assembly and all C code */
        *(.text)
    }

    /*
     4. READ-ONLY DATA
     * This holds constant variables and static strings (like "Hello from NG-OS!").
     */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /*
     5. INITIALIZED DATA
     * This holds global variables that you gave a starting value to (e.g., int x = 5;).
     */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /*
     6. UNINITIALIZED DATA (BSS)
     * This holds global variables with no starting value, AND our 16KB kernel stack.
     * The Linker doesn't actually write zeros to the file for this section to save space;
     * it just tells GRUB to carve out empty RAM for it upon boot.
     */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* * 7. THE KERNEL END MARKER (NEW)
         * We drop a symbol right here at the absolute end of all our code.
         * The C code will use this symbol as the safe starting address to build
         * the PMM Bitmap so we don't accidentally overwrite our own operating system.
         */
        _kernel_end = .;
}