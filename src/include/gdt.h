#ifndef GDT_H
#define GDT_H
#include <stdint.h>

/*
 * like the IDT, the GDT is an array of 8-byte (64-bit) entries. However,
 * the bits are arranged slightly differently because they describe memory boundaries instead of function addresses.
 */

//a single 8-byte entry in the Global Descriptor Table
struct gdt_entry_struct {
    uint16_t limit_low; // the lower 16 bits of the memory limit
    uint16_t base_low; // the lower 16 bits of the starting address
    uint8_t base_middle; // the next 8 bits of the starting address
    uint8_t access; // access flags (Executable, Ring 0, etc.)
    uint8_t granularity; //granularity flags (4KB block sizes)
    uint8_t base_high; // the final 8 bits of the starting address
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t; //alias name

//the pointer structure we hand to the CPU (just like the IDT pointer)
struct gdt_ptr_struct {
    uint16_t limit; //total size of the GDT
    uint32_t base; //physical memory address of the GDT
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t; //alias name

void gdt_init(void);
#endif

