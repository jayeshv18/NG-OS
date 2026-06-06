/*
To give the illusion that multiple programs are running at the exact same time, the kernel needs an alarm clock.
it needs a piece of hardware that screams at the CPU thousands of times a second: "STOP WHAT YOU ARE DOING! RUN THE KERNEL NOW!"
that hardware is called the PIT (Programmable Interval Timer).

The PIT listens on exactly four specific ports:
    Port 0x40: Channel 0 Data Port (This is the one wired directly to the CPU's Interrupt Request line. This is the alarm clock).
    Port 0x41: Channel 1 Data Port (Used for legacy RAM refreshing; ignore this).
    Port 0x42: Channel 2 Data Port (Wired to the PC Speaker. We can actually use this to make the motherboard beep!).
    Port 0x43: Command Register (The configuration port).

we must send a "configuration byte" to the Command Register (0x43). This byte tells the PIT how we want it to behave.
we want to tell it: "Use Channel 0, operate in Square Wave Mode (continuous ticking), and expect a 16-bit frequency number."
The mathematical hex code for this exact configuration is 0x36.

the PIT contains an internal crystal oscillator that vibrates at exactly 1,193,180 Hz.
it cannot tick faster than that. To set our own frequency (like 1000 Hz), we must send a "divisor" to the PIT.
Divisor = 1193180 / Target_Frequency

we want 1000 Hz, the divisor is 1193. 1193 is a 16-bit number, and the I/O port (0x40) can only accept 8 bits at a time, we must physically slice the divisor in half and send the Low Byte first, followed immediately by the High Byte.

before we send the divisor, we have to tell the PIT chip to open Channel 0 and get ready to receive data.
we do this by sending the hex code 0x36 to the command port 0x43.
 */
#include <stdint.h>
#include "include/io.h"
void pit_init(uint32_t frequency) {
    uint16_t divisor=1193180/frequency; //make it tick at our requested frequency.
    uint8_t low=divisor & 0xFF; //extracts bottom 8 bits
    uint8_t high=divisor>>8;
    outb(0x43, 0x36);
    outb(0x40, low);
    outb(0x40, high);
}
//  rn PIT is awake and alarming at the CPU 1000 times a second. But if we run our OS right now, it will instantly crash.
//because when the CPU hears that alarm, it drops what it is doing and blindly looks in our Interrupt Descriptor Table (IDT) for instructions on how to handle it. If the slot is empty, the CPU panics.
/*
Here is how the motherboard wiring works:
    The PIT sends its electrical signal down a wire called IRQ 0 (Interrupt Request 0).
    IRQ 0 plugs directly into the Programmable Interrupt Controller (PIC).
    When we initialized our PIC (back when we set up the IDT), we told the PIC to map its first hardware interrupt (IRQ 0) to IDT entry 32 (0x20).
    Therefore, the PIT alarm clock is officially known to the CPU as Interrupt 32.

we already wrote the C logic for the handler (timer_handler_main in our timer.c file from earlier), which successfully increments the tick variable and sends the End of Interrupt (EOI) signal to the PIC (outb(0x20, 0x20)).
to complete the circuit, we just need to tell the IDT that Interrupt 32 belongs to our timer handler.
 */

/*
(important)
motherboard's PIT chip doesn't actually know about the number 32. It just sends raw electricity down a wire called IRQ 0.
it is the PIC chip that catches that electricity, looks at the remapping rules we wrote in pic_remap(), and translates IRQ 0 into the number 32. It then shoves the number 32 directly into the CPU's pins.
pit_init(18) will alarm the cpu forcing to look into idt and from idt it is wired as handler at 32 that calls the timer_stub in asm that saves the context, call timer handler main , restore, execute and return from interrupt.
 */

