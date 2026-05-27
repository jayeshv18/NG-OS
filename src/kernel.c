#include "include/vga.h"
#include "include/idt.h"
/*
 *In user-space, main() returns an integer (like return 0;) back to the operating system.
 *But we are the operating system. There is nothing to return to!
 *
 *If kernel_main ever reaches the bottom closing bracket }, the CPU will try to execute whatever random garbage data is sitting in memory right after our kernel.
 *The CPU will panic and triple-fault.
 */
void kernel_main() {
    /* Code-Update until the next ----> , (note: this is an old code used for testing, it's commented out and not deleted to make it suitable to learning and decoding.)
    char* vga_buff=(char*)0xb8000; //a pointer called 'vga_buff' at memory address 0xB8000. motherboard is physically wired so that anything written to 0xB8000 goes straight .
    // into the video card's memory instead of regular RAM. We use (char*) to tell the compiler: "Treat this raw memory address as a row of text character boxes."
    //in VGA text mode, the screen is a grid of boxes, and each box requires 2 bytes of data.
    vga_buff[0]='K'; // Index [0] is the first byte, which controls WHAT letter is displayed in the top-left corner. We assign the character 'K' (which the compiler automatically turns into its 8-bit ASCII number, 75).
    vga_buff[1]=0x0f;// Index [1] is the second byte, which sits right next to the character in memory.
    // The hex code 0x0F is split into two numbers: The first digit (0) tells the hardware to make the background pitch black.The second digit (F) tells the hardware to paint the letter 'K' in bright white.
    ---->*/
    vga_clear_screen();
    vga_print("Hello from the NG-OS Kernel!\n");
    vga_print("System Initialized Successfully.\n");
    //vga_hex_print(0x1BADB002); //translates a raw 32-bit number into readable text by slicing it into 4-bit chunks. Uses a bitwise mask and right-shifts to decode digits right-to-left before printing via VGA.
    // for more details check vga.c for vga_hex_print.

    idt_init();

    /*
     *A normal program when your main() function finishes, it executes a return statement. This returns control back to the operating system's kernel,
     *which securely cleans up the memory and closes the process. Since we are writing the operating system itself, there is nothing left to return to.
     *Since our kernel has no host OS to return to, exiting this function would cause a fatal system crash.
     *To prevent this, we enter an infinite loop that uses the raw assembly "hlt" (Halt) instruction.
     *This safely puts the CPU into a low-power sleep state instead of spinning at 100% usage and generating heat.
     *If a hardware interrupt wakes the processor up, the loop immediately forces it back to sleep safely.
     */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}