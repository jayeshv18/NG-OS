/*
 * when GRUB queries the BIOS for memory, it gets back an array of structures.
 * we need to tell our C compiler exactly what one of these structures looks like so we can read the array.
 */
#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
// A single entry in the GRUB Memory Map
struct multiboot_mmap_entry {
    uint32_t size; //size of this specific entry (usually 20 bytes)
    uint64_t addr; //the starting physical address of this memory chunk
    uint64_t len; //the length/size of this memory chunk in bytes
    uint32_t type; //the classification code (1 = Available, 2 = Reserved)
} __attribute__((packed));
typedef struct multiboot_mmap_entry multiboot_mmap_t;

/*
 * Notice how addr and len are uint64_t (64-bit), even though our OS is only 32-bit.
 * The motherboard can easily have more than 4GB of RAM installed, which physically requires a 64-bit number to represent.
 * If we use a 32-bit integer here, the compiler will truncate the massive addresses, and our OS will go completely blind to half our RAM.
 */

//the function we gonna call from kernel
void parse_memory_map(void* mb_info);
void* memcpy(void* dest, const void* src, uint32_t size);
#endif
