#ifndef IDT_H
#define IDT_H
#include <stdint.h>
//__attribute__((packed)) directive so the compiler doesn't insert hidden memory gaps to optimize it.
//struct for a single IDT Entry (64 bits) (8bytes)
typedef struct {
    uint16_t isr_low; //lower 16 bits of the ISR's address
    uint16_t kernel_cs; //GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t reserved; //always set to 0
    uint8_t attributes; //Flags that define the entry. For a 32-bit interrupt gate, this is always 0x8E (binary 10001110).
    uint16_t isr_high; //higher 16 bits of the ISR's address
}__attribute__((packed)) idt_entry_t;
//Interrupt Request, that is the electrical signal the hardware sends to the CPU. The CPU receives the IRQ, looks at the IDT, and executes the ISR
typedef struct {
    uint16_t table_limit;
    uint32_t table_address;
} __attribute__((packed)) idt_ptr_t; //specific 48-bit pointer

/*
 *we already made struct like, idt_entry_t which is make of 64 bits ie 8bytes. so when we declare array like extern idt_entry_t idt[256];
 *we are saying the compiler that to reserve a continous block of memory that is 256*8=2048 bytes or 2KB long.
 *we do not need to use uint64_t because we already defined the total 64-bit size inside the structure itself.
 *In fact, if we tried to declare the array as extern uint64_t idt[256];, we would ruin our architecture.
*keeping it as an array of idt_entry_t structs, you can easily set a value like this:
idt[33].attributes = 0x8E;
 */
extern idt_entry_t idt[256]; // actual array of 256 IDT entries
extern idt_ptr_t idtp; // pointer structure that we will hand to the CPU

void idt_init(void);
#endif
