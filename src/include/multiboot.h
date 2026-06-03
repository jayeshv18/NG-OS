#ifndef MULTIBOOT_H
#define MULTIBOOT_H

/*
 *When an assembly program calls a C function, the C compiler assumes any parameters it needs will be sitting right at the top of the stack.
 *Because a stack is a Last-In, First-Out (LIFO) structure:
 *We pushed ebx (The Multiboot Info Pointer) first. It goes to the bottom.
 *We pushed eax (The Magic Number) second. It sits at the top.
 *Therefore, the C function reads them in reverse order. The first parameter is eax, and the second is ebx.
 */

/*
 * GRUB didn't just hand us a single number;
 * it handed us a massive data structure containing the bootloader name, the kernel command line, video information, and the Memory Map.
 *
 * The Briefcase (EBX) is just a raw physical memory address (like 0x10000). If we just gave that number to C, C wouldn't know what it meant.
 * By defining multiboot_info_t, we are giving the C compiler a blueprint. we are telling it: "If you go to address 0x10000, the first 4 bytes are flags.
 * The next 4 bytes are lower memory. If you skip down 44 bytes, you will find mmap_addr, which tells you where the actual RAM map begins."
 * The __attribute__((packed)) part forces the compiler not to add any hidden padding bytes, ensuring it exactly matches GRUB's physical layout.
 */
#include <stdint.h>
#define MULTIBOOT_NUMBER_CHECK 0x2BADB002 //specific verification magic number GRUB passes to prove it booted successfully

typedef struct {
    uint32_t flags;         // first 4 bytes are flags. Indicates which of the fields below are valid and filled
    uint32_t mem_lower;     // next 4 bytes are lower memory. Size of lower base memory (Conventional RAM) in Kilobytes
    uint32_t mem_upper;     // Size of upper extended memory (RAM past 1MB) in Kilobytes
    uint32_t boot_device;   // Physical drive information (like BIOS drive numbers)
    uint32_t cmdline;       // Physical memory pointer to the kernel parameters string
    uint32_t mods_count;    // Total number of external boot modules loaded
    uint32_t mods_addr;     // Memory address pointing to the loaded modules array

    // Elf Section Header Table info (used to help map symbols during crashes)
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    //Hardware Memory Mapping Fields
    uint32_t mmap_length;   // Total size of the physical motherboard memory map in bytes / How long the memory map is
    //the actual RAM map begins from mmap_addr its 44bytes down from the flag.
    uint32_t mmap_addr;     // The physical address in RAM where the memory map list begins / physical memory address of the memory map

    //we found the map of RAM so we need to rush to memory.c , the map is an array and its total size in bytes is in mmap_length. So we need to read it.
} __attribute__((packed)) multiboot_info_t;

#endif
