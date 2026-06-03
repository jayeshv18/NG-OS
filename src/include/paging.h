#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>

/*
* In C, when you write that line without the word extern or without putting it inside a struct, you are not just describing a blueprint. You are telling the compiler: "Physically allocate 4096 bytes of memory right here, right now, and name it page_table."
Because this is a header file (.h), every single time a different .c file includes it (e.g., kernel.c, paging.c, memory.c), the compiler will create a brand new, separate 4096-byte array named page_table.
When the Linker tries to glue your OS together, it will see five different arrays all named page_table, throw a Multiple Definition Error, and refuse to build the OS.
 */

typedef struct {
 //this is exactly 4096 bytes long. It holds 1024 individual entries.
 //each entry is a 32-bit integer containing:
 // [ Top 20 Bits: Physical Block Address ] + [ Bottom 12 Bits: Security Flags ]
 uint32_t entries[1024];
} page_table; //an array of 1024 integers (Each integer holds a physical address + 12 flags).

typedef struct {
// this is also exactly 4096 bytes long. It holds 1024 individual entries.
// each entry is a 32-bit integer containing:
// [ Top 20 Bits: Physical Address of a Page Table ] + [ Bottom 12 Bits: Security Flags ]
 uint32_t entries[1024];
} page_directory; //array of 1024 integers (Each integer holds the physical address of a Page Table + 12 flags).
void paging_init();
#endif
