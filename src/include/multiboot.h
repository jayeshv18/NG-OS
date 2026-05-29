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
 */
#include <stdint.h>
#define MULTIBOOT_NUMBER_CHECK 0x2BADB002 //specific verification magic number GRUB passes to prove it booted successfully

typedef struct {
    uint32_t flags;         // Indicates which of the fields below are valid and filled
    uint32_t mem_lower;     // Size of lower base memory (Conventional RAM) in Kilobytes
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
    uint32_t mmap_addr;     // The physical address in RAM where the memory map list begins / physical memory address of the memory map
} __attribute__((packed)) multiboot_info_t;

#endif
