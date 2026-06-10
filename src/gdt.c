#include "include/gdt.h"

/*
* We need to create three specific entries in our GDT to satisfy the CPU:
* The Null Descriptor (Index 0): The Intel architecture demands that the absolute first entry in the GDT must be completely zeroed out. It's a safety feature to catch null pointers.
* The Code Segment (Index 1 / Offset 0x08): This is the executable sandbox where our kernel lives.
* The Data Segment (Index 2 / Offset 0x10): This is where our kernel's variables and stack live.
*/

//we need 3 entries: Null, Code, Data ...(more if required)
gdt_entry_t gdt_entries[6];
tss_entry_t tss_entry;//global TSS ledger
gdt_ptr_t gdt_ptr;
//asm func to forcefully load the table
extern void gdt_flush(uint32_t);

//We carve out 4096 bytes of permanent RAM for the TSS to use
//must give the TSS a real, physical block of memory to use as the emergency kernel stack.
uint8_t tss_kernel_stack[4096]; //we need this stack because to hold the page fault error code or any error code from ring 3
/*
when the hardware MMU catches program_A trying to execute kernel code, it triggers a Page Fault.
The CPU realizes it must jump to our isr14 handler to deal with the error.
The CPU cannot use the User Stack to push the Error Code. If the CPU pushed the sensitive Ring 0 Return Address and the Error Code onto program_A's stack,
a malicious user program could inspect that stack, find the kernel's memory addresses, and use them to break out of the sandbox.
 */


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
//initialization function to zero out the TSS, set up the kernel stack
//the TSS (Task State Segment) is literally just a piece of paper we tape to the CPU that says: "If an emergency happens while you are in Ring 3, here is a safe Ring 0 stack you can use."
void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);
    //add the TSS descriptor to the GDT (Access byte 0x89 means a 32-bit TSS)
    gdt_set_gate(num, base, limit, 0x89, 0x00);
    //zero out the struct
    for (int i = 0; i < sizeof(tss_entry); i++) {
        ((uint8_t*)&tss_entry)[i] = 0;
    }
    tss_entry.ss0  = ss0;  // Set the kernel stack segment
    tss_entry.esp0 = esp0; // Set the kernel stack pointer
    //set the IO map base to the size of the TSS to disable it
    tss_entry.iomap_base = sizeof(tss_entry);
}


void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    gdt_set_gate(0,0,0,0,0);
    gdt_set_gate(1,0,0xFFFFFFFF,0x9A, 0xCF); //The Code Segment (Starts at 0, spans 4GB, Executable, Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); //The Data Segment (Starts at 0, spans 4GB, Non-Executable, Ring 0)
    //ring 3 Sandbox
    //user Code and User Data. These entries will look almost identical to your kernel entries, except their security privilege bits will be hardcoded to 3 instead of 0.
    gdt_set_gate(3,0,0xFFFFFFFF,0xFA,0xCF); //0xFA (User Code Segment)
    /*
    Bit 1 (Readable): 1 -> The code can be read by the CPU.
    Bit 2 (Conforming): 0 -> Lower privilege rings cannot jump into this code.
    Bit 3 (Executable): 1 -> This is a Code segment (not a Data segment).
    Bit 4 (Descriptor Type): 1 -> This is a memory segment, not a system segment.
    Bits 5 & 6 (Privilege Level): 11 -> This is the lock. 11 in binary is 3 in decimal. This tells the CPU: "This code is Ring 3 User Space."
    Bit 7 (Present): 1 -> The segment exists in memory.
     */
    gdt_set_gate(4,0,0xFFFFFFFF,0xF2,0xCF); //0xF2 (User Data Segment)
    //Bit 3 (Executable): 0 -> This is a Data segment. The CPU will actively block any attempt to execute code here (preventing buffer overflow execution attacks).

    //tss initialization
    write_tss(5,0x10,(uint32_t)&tss_kernel_stack[4096]); //5 is the array index, 0x10 is your Kernel Data Segment

    gdt_flush((uint32_t)&gdt_ptr); //the CPU to load our new table

    /*
     * Granularity byte tells the CPU's memory management unit how to measure the memory (e.g., "Are we measuring in 1-byte blocks or 4-Kilobyte blocks?
     * Is this a 32-bit segment or a 16-bit segment?"). User programs are 32-bit and use 4KB pages, just like the kernel.
     */

    /*
    Index 0: Null
    Index 1: Kernel Code
    Index 2: Kernel Data
    Index 3: User Code
    Index 4: User Data
     */
}
