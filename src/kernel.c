

#include "include/vga.h"
#include "include/idt.h"
#include "include/multiboot.h"
#include "include/gdt.h"
#include "include/memory.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/heap.h"
#include "include/pit.h"
#include "include/task.h"
//we declare that this symbol exists outside of our C code (in the linker script)
extern uint32_t _kernel_end;
extern void jump_to_usermode(void (*entry_point)(), uint32_t user_esp);
extern void tss_flush();

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

void program_A() {
    for (;;) {
        __asm__ volatile (
            "mov $1, %%eax\n"     // System Call Number 1 (Print)
            "mov $'A', %%ebx\n"   // Parameter: The character 'A'
            "int $0x80\n"         // The Drop into Ring 0
            : : : "eax", "ebx"
        );

        int count=5;
        while (count>1) { count--; }
        for(volatile uint32_t i=0; i<1000000; i++) {}
    }
}
void program_A_end() {}

void program_B() {
    for (;;) {
        __asm__ volatile (
            "mov $1, %%eax\n"     // System Call Number 1 (Print)
            "mov $'B', %%ebx\n"   // Parameter: The character 'B'
            "int $0x80\n"         // The Drop into Ring 0
            : : : "eax", "ebx"
        );
        for(volatile uint32_t i=0; i<1000000; i++) {}
    }
}

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


    //vga_hex_print(0x1BADB002); //translates a raw 32-bit number into readable text by slicing it into 4-bit chunks. Uses a bitwise mask and right-shifts to decode digits right-to-left before printing via VGA.
    // for more details check vga.c for vga_hex_print.

    gdt_init();  //must be called before idt
    klog_ok("GDT Loaded Successfully!\n");
    tss_flush();

    idt_init(); //must be called after gdt
    klog_ok("IDT Initialized. Hardware interrupts active.\n");
    pit_init(18);

    //we get the physical memory address of the symbol using the Address-Of operator in the last of linker.ld
    uint32_t end_address=(uint32_t)&_kernel_end;
    //add lower and upper memory (in KB) and multiply by 1024 to get total Bytes
    uint32_t total_ram_size = (mb_info->mem_lower + mb_info->mem_upper) * 1024;

    //we pass that safe address into our PMM
    pmm_init(end_address, total_ram_size); //_kernel_end is a symbol created by the linker, not a real C variable, we must use & here to get the raw physical address the linker assigned to it.
    klog_ok("PMM Initialized.\n");

    parse_memory_map(mb_info); //read hardware mem

    //security bug fixed...
    /*
     *the BIOS usually marks the first 640KB as "Available", but address 0x0
     *contains the legacy Interrupt Vector Table. We lock it manually so
     *pmm_alloc_block() never returns a NULL pointer.
    */
    pmm_lock_memory(0x0); //lock the 0x0 block to know more about it read at pmm.c

    /*----> old code
    vga_print("Total RAM Blocks (4KB): ");
    vga_print_dec(pmm_get_total_blocks());
    vga_print("\nLocked Blocks: ");
    vga_print_dec(pmm_get_used_blocks());
    vga_print("\nFree Blocks: ");
    vga_print_dec(pmm_get_total_blocks() - pmm_get_used_blocks());
    ---->*/

    paging_init();
    klog_ok("Paging Initialized.\n");
    heap_init();
    klog_ok("Heap Initialized.\n");
    task_init(); //captures the current kernel into PID 1
    klog_ok("Task Initialized.\n");

    os_greeting("NG-OS :)");

    uint32_t prog_a_page = (uint32_t)program_A & ~0xFFF;
    map_page(prog_a_page, prog_a_page);
    uint32_t prog_b_page = (uint32_t)program_B & ~0xFFF;
    map_page(prog_b_page, prog_b_page);

    /*
    the hardware Memory Management Unit (MMU) does not understand individual
    C functions or variables. It applies security rules to physical memory in
    rigid 4,096-byte (4KB) chunks called "Pages".

    right now, the entire Kernel (including program_A and program_B) is locked
    to Ring 0 (Supervisor Only: | 3). If we drop into Ring 3 and the CPU tries
    to read the first instruction of program_A, the MMU will instantly block it
    and throw a Page Fault. Because our TSS isn't fully robust yet, that Page
    fault cascades into a fatal Triple Fault.
    we must slide exactly two keys under the door, telling the MMU to grant
    ring 3 access to the specific 4KB blocks where these two programs live.

    let's pretend program_A lives at memory address 0x001054A2.
    we cannot unlock just 0x4A2. We must unlock the entire 4KB block starting at 0x00105000.
    ~0xFFF is a bitwise mask. 0xFFF is twelve 1s. ~ flips it into a wall of 1s ending in twelve 0s.
    by using the Bitwise AND (&) operator against the raw memory pointer, we violently
    wipe out the bottom 12 bits of the address. This perfectly rounds the address down
    to the absolute starting boundary of its 4KB physical page.
    */

    //5-Plate Ring 3 Stacks for our User Programs
    create_task(program_A); //forges the stack for A (PID 2)
    create_task(program_B); //forges the stack for B (PID 3)

    extern page_directory* kernel_directory;
    for(int i=0; i<1024; i++) {
        kernel_directory->entries[i] |= 7;
    }
    // Flush the TLB
    uint32_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3_val));
    __asm__ volatile("mov %0, %%cr3" :: "r" (cr3_val));

    klog_ok("Multitasking Engine Engaged. Waiting for first tick...");

    /*
     *Operating System architecture, we must never turn on hardware interrupts (sti) until our kernel is 100% fully initialized and ready to handle them.
     *if we dont follow this, it'll run into Hardware Race Condition.
     *idt_init() runs. At the very end of this function, we wrote __asm__ volatile("sti");. This turns the CPU starts listening to the hardware.
     *pit_init() runs. The motherboard alarm clock starts firing 1000 times a second.
     *OS moves on to initialize Paging, the Heap, etc.
     *The PIT alarm fires.
     *CPU instantly drops what it is doing and jumps into our timer_stub.
     our Assembly code runs: mov eax, [current_task].
     *but our kernel hasn't reached task_init() yet! Because task_init() hasn't run, the global variable current_task is NULL (Memory Address 0x0).
     *assembly function takes that 0x0, adds 4 to it, and writes the CPU's stack pointer into memory address 0x00000004. Then, it reads the "next" task from address 0x00000008 (which is just random garbage RAM), shoves that garbage into the esp register, pops garbage into the CPU, and Triple Faults.
     */
    __asm__ volatile("sti"); //on interrupts so the CPU can hear the PIT timer
    /*
     *A normal program when our main() function finishes, it executes a return statement. This returns control back to the operating system's kernel,
     *which securely cleans up the memory and closes the process. Since we are writing the operating system itself, there is nothing left to return to.
     *Since our kernel has no host OS to return to, exiting this function would cause a fatal system crash.
     *To prevent this, we enter an infinite loop that uses the raw assembly "hlt" (Halt) instruction.
     *This safely puts the CPU into a low-power sleep state instead of spinning at 100% usage and generating heat.
     *If a hardware interrupt wakes the processor up, the loop immediately forces it back to sleep safely.
     */


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