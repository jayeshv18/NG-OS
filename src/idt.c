#include "include/idt.h"
#include "include/io.h"

extern void keyboard_handler(); //assembly stub (function) so C knows it exists.

idt_entry_t idt[256];
idt_ptr_t idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    /*
     *num is the index in the array (0-255).
     *base is the full 32-bit memory address of our C function.
     *selector is the GDT selector (0x08). (privilege level)
     *flags is the attribute byte (0x8E).
     */

    //will do the bit-shifting necessary to chop the 32-bit base address into two 16-bit halves to assign them to isr_low and isr_high.
    idt[num].isr_low = (base & 0xFFFF);
    idt[num].kernel_cs = selector;
    idt[num].reserved = 0;
    idt[num].attributes = flags;
    idt[num].isr_high = (base >> 16) & 0xFFFF; // The '& 0xFFFF' is a safety cast


}

// The PIC sits on these hardware ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
/*
 * pic_remap(): The Hardware Retraining Sequence
 * * THE PROBLEM:
 * In 1981, the BIOS was programmed to map physical hardware interrupts (IRQs)
 * to IDT entries 8 through 15.
 * However, in modern 32-bit architecture, Intel rigidly reserved IDT entries
 * 0 through 31 for Fatal CPU Exceptions (like Double Faults or Divide by Zero).
 * * If we turn on hardware interrupts without remapping the PIC, the motherboard
 * timer will tick (IRQ 0), the PIC will send it to IDT entry 8, and the CPU
 * will mistakenly believe it just suffered a fatal Double Fault. This causes
 * a Triple Fault and instantly reboots the machine.
 * * THE SOLUTION:
 * We must send Initialization Command Words (ICWs) to the PIC chips to
 * shift their output signals completely out of the 0-31 danger zone.
 */
void pic_remap() {
    uint8_t a1, a2;

    // Save the current masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // ICW1: Start the initialization sequence in cascade mode
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2: The vital step. Set Master PIC vector offset to 0x20 (32)
    outb(PIC1_DATA, 0x20);
    // Set Slave PIC vector offset to 0x28 (40)
    outb(PIC2_DATA, 0x28);

    // ICW3: Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04);
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02);

    // ICW4: Set 8086/88 (x86) mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Restore the saved masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}


void idt_init(void) {
    idtp.table_limit = 256 * sizeof(idt_entry_t) - 1; // Size of the table
    idtp.table_address = (uint32_t)&idt; // Memory address of the table
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i,0,0,0); //clean the ram if any garbage left by previous storing data.
    }
    //registering the keyboard interrupt handler at index 33 (0x21)
    idt_set_gate(33, (uint32_t)keyboard_handler, 0x08, 0x8E);
    /*
     *0x8E = 10001110, 7th bit: Is this handler currently loaded in RAM and ready to use? 1 yes, 0 no.
     *6th and 5th bit (00): What security clearance (Ring Level) does a piece of software need to trigger this interrupt using software commands. (00) is kernel.
     *4th bit: Is this a data storage segment or an executable gate? 0 for interrupt handler, it must always be 0, telling the CPU this is a "Gate" (a doorway to executable code), not a storage variable.
     *bits 3rd, 2nd, 1st (1110): What specific type of Gate is this? Intel defines several types (like Task Gates, 16-bit Interrupt Gates, 32-bit Trap Gates, etc.). 1110. In Intel's hardware manual, the binary sequence 1110 translates to a 32-bit Interrupt Gate.
     *Convert binary 10001110 back to hexadecimal, and we get exactly 0x8E
    */

    pic_remap();//move the hardware signals out of the danger zone!
    //Mute all hardware signals except the Keyboard (IRQ 1)
    outb(0x21, 0xFD); // 0x21 is the Master PIC Data Port
    outb(0xA1, 0xFF); // 0xA1 is the Slave PIC Data Port
    // inline asm: Load the table rules into the CPU's internal IDTR register
    // The "m" constraint tells GCC to pass the memory address of our idtp structure.
    __asm__ __volatile__("lidt %0" : : "m" (idtp)); //find the exact physical memory address of idtp in RAM, and shove that memory address into the %0 placeholder
    //calculates that idtp is sitting at memory address 0x10500, and silently translates our C code into this pure, raw assembly instruction inside the final binary:
    //lidt [0x10500]
    // Inline Assembly: Turn the CPU back on to intercept hardware device signals
    __asm__ volatile("sti");
}