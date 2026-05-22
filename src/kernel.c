#include "include/serial.h"
#include "include/limine.h" // the bootloader
#include <stdint.h>  // Gives us uint64_t
#include <stdbool.h> // Gives us true/false
#include <stddef.h> // We need this for the NULL pointer definition

/*
* Forces this request into a custom ".requests" section so the Limine bootloader can scan and find it.
* The 'used' attribute prevents the compiler from optimizing away or deleting this variable as dead code.
'volatile' tells the compiler that the bootloader will modify this data outside of our C code's awareness.
 */
// __attribute__((used)) forces GCC to keep this in the final binary.
__attribute__((aligned(8)))
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};


/*
 *In user-space, main() returns an integer (like return 0;) back to the operating system.
 *But we are the operating system. There is nothing to return to!
 *
 *If kernel_main ever reaches the bottom closing bracket }, the CPU will try to execute whatever random garbage data is sitting in memory right after our kernel.
 *The CPU will panic and triple-fault.
 */
void kernel_main(void) {
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {// Limine modifies base_revision[2] during boot. If it is 0, we are unsupported.
        for (;;) { //if the bootloader is too old or doesn't support, we cant safely boot, we use infinite loop to put CPu on halt and prevent crash.
            __asm__ volatile ("hlt");
        }
    }
    // if it is true then the kernel is alive and welcome.
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    serial_print("Welcome to NG-OS:)\n");
    serial_print("Framebuffer successfully acquired!\n");

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