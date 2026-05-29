#include "include/gdt.h"

/*
* We need to create three specific entries in our GDT to satisfy the CPU:
* The Null Descriptor (Index 0): The Intel architecture demands that the absolute first entry in the GDT must be completely zeroed out. It's a safety feature to catch null pointers.
* The Code Segment (Index 1 / Offset 0x08): This is the executable sandbox where our kernel lives.
* The Data Segment (Index 2 / Offset 0x10): This is where our kernel's variables and stack live.
*/

//we need 3 entries: Null, Code, Data
gdt_entry_t gdt_entries[3];
gdt_ptr_t gdt_ptr;
//asm func to forcefully load the table
extern void gdt_flush(uint32_t);

// The helper function to calculate the bits and build a single entry
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF); //takes a 32-bit address and forcefully chops off the top half, saving only the bottom 16 bits.
    gdt_entries[num].base_middle = (base >> 16) & 0xFF; //shifts the integer right by 16 bits, sliding the middle chunk down to the bottom so we can grab 8 bits of it.
    gdt_entries[num].base_high = (base >> 24) & 0xFF; //shifts the integer right by 24 bits, sliding the absolute highest 8 bits down to the bottom to be saved.

    gdt_entries[num].limit_low = (limit & 0xFFFF); //grabs the bottom 16 bits of the memory size.
    gdt_entries[num].granularity = (limit >> 16) & 0x0F; //grabs the top 4 bits of the memory size and carefully wedges them into the bottom half of the granularity byte.
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    gdt_set_gate(0,0,0,0,0);
    gdt_set_gate(1,0,0xFFFFFFFF,0x9A, 0xCF); //The Code Segment (Starts at 0, spans 4GB, Executable, Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); //The Data Segment (Starts at 0, spans 4GB, Non-Executable, Ring 0)
    gdt_flush((uint32_t)&gdt_ptr); //the CPU to load our new table
}