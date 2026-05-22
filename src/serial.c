#include "include/io.h"
void serial_write_char(char c) {
    outb(0x3F8, c);
}
/*
 * When we type a message directly into your code, like serial_print("Hello");,
 * that text is placed in section of our compiled OS called the .rodata (Read-Only Data) section.
 *
 * If our function just used char* str (without const),
 * we could accidentally write a line of code inside the function like *str = 'X'; to try and change "Hello" to "Xello".
 *
 * Because the hardware explicitly locks the .rodata memory page to protect it, the moment the CPU executes that change,
 * it will instantly panic, throw a Page Fault, and completely crash your custom operating system.
 * Adding const tells the compiler: "This text is strictly look, don't touch."
 */
void serial_print(const char* str) {
    while (*str!='\0') {// Loop through every character until we hit the null terminator '\0'
        outb(0x3F8, *str); // Send the current letter to COM1 ()
        str++; // Move to the next letter
    }
}