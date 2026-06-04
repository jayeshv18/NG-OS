#include "include/vga.h"
#include "include/idt.h"
#include "include/multiboot.h"
#include "include/gdt.h"
#include "include/memory.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/heap.h"
//we declare that this symbol exists outside of our C code (in the linker script)
extern uint32_t _kernel_end;

/*
 *In user-space, main() returns an integer (like return 0;) back to the operating system.
 *But we are the operating system. There is nothing to return to!
 *
 *If kernel_main ever reaches the bottom closing bracket }, the CPU will try to execute whatever random garbage data is sitting in memory right after our kernel.
 *The CPU will panic and triple-fault.
 */

//because the x86 stack operates on a Last-In, First-Out (LIFO) basis, our C function will read the parameters in the exact opposite order they were pushed.
/*
 * 'grub_magic_number' reads EAX (Pushed 2nd, sitting at the absolute top of the stack).
 * 'mb_info' reads EBX (Pushed 1st, sitting directly underneath EAX as a raw RAM pointer). contains the physical address of GRUB's Memory Map
 */
void kernel_main(uint32_t grub_magic_number, multiboot_info_t* mb_info) {
    /* Code-Update until the next ----> , (note: this is an old code used for testing, it's commented out and not deleted to make it suitable to learning and decoding.)
    char* vga_buff=(char*)0xb8000; //a pointer called 'vga_buff' at memory address 0xB8000. motherboard is physically wired so that anything written to 0xB8000 goes straight .
    // into the video card's memory instead of regular RAM. We use (char*) to tell the compiler: "Treat this raw memory address as a row of text character boxes."
    //in VGA text mode, the screen is a grid of boxes, and each box requires 2 bytes of data.
    vga_buff[0]='K'; // Index [0] is the first byte, which controls WHAT letter is displayed in the top-left corner. We assign the character 'K' (which the compiler automatically turns into its 8-bit ASCII number, 75).
    vga_buff[1]=0x0f;// Index [1] is the second byte, which sits right next to the character in memory.
    // The hex code 0x0F is split into two numbers: The first digit (0) tells the hardware to make the background pitch black.The second digit (F) tells the hardware to paint the letter 'K' in bright white.
    ---->*/
    vga_clear_screen();
    if (grub_magic_number!=MULTIBOOT_NUMBER_CHECK) { //checking hardcoded hex value defined by the Free Software Foundation.
        vga_print("FATAL ERROR: Invalid Multiboot Magic Number.\n");
        return; // Halt the kernel
    }
    vga_print("Hello from the NG-OS Kernel!\n");
    vga_print("System Initialized Successfully.\n");
    vga_print("Multiboot check passed. GRUB is online.\n");

    //vga_hex_print(0x1BADB002); //translates a raw 32-bit number into readable text by slicing it into 4-bit chunks. Uses a bitwise mask and right-shifts to decode digits right-to-left before printing via VGA.
    // for more details check vga.c for vga_hex_print.

    vga_print("Building GDT...\n");
    gdt_init();  //must be called before idt
    vga_print("GDT Loaded Successfully!\n");

    vga_print("Building IDT...\n");
    idt_init(); //must be called after gdt
    vga_print("IDT Initialized. Hardware interrupts active.\n");

    //we get the physical memory address of the symbol using the Address-Of operator in the last of linker.ld
    uint32_t end_address=(uint32_t)&_kernel_end;
    //add lower and upper memory (in KB) and multiply by 1024 to get total Bytes
    uint32_t total_ram_size = (mb_info->mem_lower + mb_info->mem_upper) * 1024;

    vga_print("Building Physical Memory Manager...\n");
    //we pass that safe address into our PMM
    pmm_init(end_address, total_ram_size); //_kernel_end is a symbol created by the linker, not a real C variable, we must use & here to get the raw physical address the linker assigned to it.
    vga_print("PMM Initialized. All blocks locked.\n");

    vga_print("Scanning BIOS Memory Map...\n");
    parse_memory_map(mb_info); //read hardware mem

    //security bug fixed...
    /*
     *the BIOS usually marks the first 640KB as "Available", but address 0x0
     *contains the legacy Interrupt Vector Table. We lock it manually so
     *pmm_alloc_block() never returns a NULL pointer.
    */
    pmm_lock_memory(0x0); //lock the 0x0 block to know more about it read at pmm.c

    vga_print("Total RAM Blocks (4KB): ");
    vga_print_dec(pmm_get_total_blocks());
    vga_print("\nLocked Blocks: ");
    vga_print_dec(pmm_get_used_blocks());
    vga_print("\nFree Blocks: ");
    vga_print_dec(pmm_get_total_blocks() - pmm_get_used_blocks());

    vga_print("\n--- ALLOCATOR TEST ---\n");
    uint32_t block1 = pmm_alloc_block();
    vga_print("Allocated Block 1 at: ");
    vga_print_hex_64(block1); // Remember our 64-bit print tool!
    vga_print("\n");

    uint32_t block2 = pmm_alloc_block();
    vga_print("Allocated Block 2 at: ");
    vga_print_hex_64(block2);
    vga_print("\n");

    vga_print("Freeing Block 1...\n");
    pmm_free_memory(block1);

    uint32_t block3 = pmm_alloc_block();
    vga_print("Allocated Block 3 at: ");
    vga_print_hex_64(block3);
    vga_print("\n");

    vga_print("\n--- FINAL DASHBOARD ---\n");
    vga_print("Locked Blocks: ");
    vga_print_dec(pmm_get_used_blocks());
    vga_print("\n-----------------------\n");

    paging_init();
    vga_print("\n--- INITIATING KERNEL HEAP ---\n");
    heap_init();
    vga_print("Heap Master Pointer aligned at Virtual 16MB.\n");

    // Test 1: Ask for 50 bytes
    void* ptr1 = kmalloc(50);
    vga_print("Allocated 50 bytes. User Pointer 1: ");
    vga_print_hex_64((uint32_t)ptr1);
    vga_print("\n");

    // Test 2: Ask for 100 bytes
    void* ptr2 = kmalloc(100);
    vga_print("Allocated 100 bytes. User Pointer 2: ");
    vga_print_hex_64((uint32_t)ptr2);
    vga_print("\n");

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