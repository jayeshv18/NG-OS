#include <stdint.h>
#include "include/vga.h"
#include "include/io.h"

// A simple US QWERTY layout map (Lower case only)

//we use an array map. Because 0x1E equals 30 in decimal, if we create an array where the 30th item is the letter 'a', we can instantly translate the hardware signal into text!
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',
  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*', 0, ' '
};

void keyboard_handler_main() {
    //first need to check the status
    uint8_t keyboard_status=inb(0x64); //motherboard's keyboard status sits at port 0x64.
    if(keyboard_status & 0x01) { //this should be 1
        //if the lowest bit (bit 0) of that status variable is a 1 means the user pressed a key.
        uint8_t keystroke_hold=inb(0x60); //data is ready, use inb to read port 0x60. This port holds the raw numerical ID of the key that was pressed.
        if (keystroke_hold < 128) {
            char c=kbd_us[keystroke_hold];
            //vga_print function was designed to print strings (arrays of characters), not a single char. WE cannot just pass c to it.
            char str[2] = {c, '\0'};
            vga_print(str);
        }
    }
    outb(0x20,0x20); //programmable interrupt controller (PIC) acknowledgement, if not done then the PIC will freeze and never send another keystroke.
    //0x20 (The Port): This is the Command Register for the Master PIC chip.
    //0x20 (The Value): In binary, this is 00100000. This specific bit pattern is the hardware command for End of Interrupt (EOI).
    /*
     * The PIC is a simple, dumb piece of silicon. It doesn't watch our CPU execute instructions. As far as the PIC is concerned, we are still on the phone with the keyboard.
     * It will continue to block all other hardware signals forever. our OS will appear completely frozen. we can mash the keyboard, but no signals will ever reach the CPU.
     *
     * If we forget this single line of code, our custom OS will boot, let us type exactly one letter, and then silently hang until we restart the machine.
     */
}