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


//Task State Segment (TSS).
//to survive an interrupt in Ring 3, we must give the CPU a TSS ledger. We only need one TSS for the entire operating system.
//TSS hardware structure. It is 104 bytes long, but we actually only care about two variables: esp0 (the Ring 0 stack pointer) and ss0 (the Ring 0 segment).
typedef struct tss_entry_struct {
    uint32_t prev_tss;   //the previous TSS - if we used hardware task switching (we don't)
    uint32_t esp0;       //the stack pointer to load when we change to kernel mode.
    uint32_t ss0;        //the stack segment to load when we change to kernel mode.
    uint32_t esp1;       //unused...
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

#endif

