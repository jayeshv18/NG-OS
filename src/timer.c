#include <stdint.h>
#include "include/io.h"
#include "include/vga.h"
uint32_t tick=0; //tracks total system uptime.
void timer_handler_main() {
    tick++;
    /*Code-Update until the next ----> this code was writtten to check the asynchronous multiptasking between the timer and keyboard.
    if (tick%18==0) {
        vga_print(".");
    }
    ----->
    */
    outb(0x20, 0x20); //End of Interrupt to the Master PIC command port
//OS still needs to count the ticks internally; it just shouldn't spam the screen.
}