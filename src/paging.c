#include "include/paging.h"

#include "include/pmm.h"
#include "include/vga.h"
// in this file, we need to create the global variable that will hold our one and only Kernel Page Directory.
page_directory* kernel_directory=0;
//the CPU's MMU strictly requires that every Page Directory and every Page Table must be mathematically perfectly aligned to a 4096-byte boundary in physical RAM.
//if we just use standard C to say page_directory_t kernel_dir;, the GCC compiler will place it wherever it wants in memory (e.g., address 0x105004).
//If the address doesn't end in 000, the MMU's bit-shifting math will completely shatter, and the CPU will Triple Fault instantly when we turn it on.

/*
 * remember that pmm_alloc_block() returns 0xFFFFFFFF if the physical RAM is completely exhausted.
 * if we don't catch that, our OS will attempt to build its master memory map at address 0xFFFFFFFF, which will instantly crash the motherboard.
 * because we are using pmm_alloc_block(), the PMM's internal math mathematically guarantees that the memory address it hands back is a perfect multiple of 4096.
 *
 * just like the PMM's "Default Deny" loop where we wrote 0xFFFFFFFF to lock everything, the Page Directory needs its own lockdown loop.
 * if we don't clear it, the array will contain whatever random garbage data was left in that RAM chip when the computer powered on, and the CPU will try to execute it.
 */

void paging_init() {
    uint32_t directory_address=pmm_alloc_block(); //requesting a perfectly 4KB-aligned physical block from our allocator
    //security guard to check if we ran off ram...
    if (directory_address==0xFFFFFFFF) {
        klog_err("FATAL OS ERROR: Out of physical memory for Page Directory!\n");
        for (;;) {
            __asm__ volatile("hlt");//system halt to prevent crash
        }
    }
    //casting the raw physical address into our struct blueprint
    kernel_directory = (page_directory*)directory_address;

    for (int i=0;i<1024;i++) {
        // We set the entry to 0x02. Why?
        // Bit 0 (Present) = 0 (This tells the CPU: "This map is empty")
        // Bit 1 (Read/Write) = 1 (This tells the CPU: "When we do fill this, allow writing")
        // Bit 2 (User/Supervisor) = 0 (Ring 0 Kernel Only)
        kernel_directory->entries[i] = 0x00000002;
    }
    klog_ok("Page Directory aligned & initialized!\n");

    //identity mapping
    /*
     * if we flip the MMU switch without this, the CPU will try to read the next instruction at 0x100000, realize it has no map for it, and the system will instantly die.
     * we need to create a map that tells the CPU: "Hey, Virtual Address 0x100000 is located exactly at Physical Address 0x100000."
     * by mapping the first 4 Megabytes of memory exactly to itself, the kernel won't even notice when physical reality is replaced by virtual reality. It will just keep executing.
     */

    uint32_t page_table_address=pmm_alloc_block();
    if (page_table_address==0xFFFFFFFF) {
        klog_err("FATAL OS ERROR: Out of physical memory for Page Table!\n");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }
    page_table* first_page_table = (page_table*)page_table_address;

    //fill the Page Table
    for (int i=0;i<1024;i++) {
        uint32_t phys_addr=i*4096; //calculate the physical address (0x0, 0x1000, 0x2000...)
        // bitwise OR with 3 (Binary: 00000011)
        // bit 0: Present (1)
        // bit 1: Read/Write (1)
        first_page_table->entries[i] = phys_addr | 3; //single Page Table has exactly 1024 entries (first_page_table->entries[i]).
        //each entry covers one 4KB block. 1024 * 4096 = 4,194,304 bytes (Exactly 4 Megabytes).
    }
    //link the Page Table into the Page Directory. We put it in slot [0] because it covers the bottom 4MB of RAM.
    kernel_directory->entries[0]=page_table_address | 3;

    //we need ti turn on the MMU, which is controlled by CR3 register and it can be turned on using asm.
    vga_print("Flipping MMU Switch.\n");
    //load CR3 and flip CR0
    __asm__ volatile (
        //move the physical address of our Directory into EAX
        "mov %0, %%eax\n"
        //load EAX into CR3 (The CPU now knows where the map is)
        "mov %%eax, %%cr3\n"
        // read the current state of CR0 into EAX
        "mov %%cr0, %%eax\n"
        // bitwise OR EAX with 0x80000000 (This sets the 31st bit to 1)
        "or $0x80000000, %%eax\n"
        // write the modified EAX back into CR0 (PAGING IS NOW ON)
        "mov %%eax, %%cr0\n"
        :
        : "r" (directory_address) //pass our directory address to %0
        : "eax"
    );

    klog_ok("Paging Active.\n");
}

// to handle the pagefaults...
//our assembly stub (isr14) has safely caught the CPU and handled the stack, we need to write the C function that it calls: page_fault_handler.
//when the CPU throws Exception 14, it takes the exact Virtual Address that caused the crash and physically locks it inside a special CPU register called CR2.
//we need to read CR2, print the address to the screen using your VGA drivers, and halt the system so we can read the telemetry.
void page_fault_handler() {
    uint32_t fault_address;
    //reading the cr2 reg to find the exact address
    __asm__ volatile ("mov %%cr2, %0" : "=r" (fault_address)); // "%% says the complier this is an actual cpu register" , %0 is a placeholder : A placeholder representing the first variable listed in the output operands (which is faulting_address)
    //=r: Tells the compiler to automatically choose any available general-purpose CPU register (like eax or rbx) to temporarily hold the data, and write-only (=) into the variable.

    klog_err("\nPAGE FAULT DETECTED\nSystem attempted to access unmapped address:");
    vga_print_hex_64(fault_address);
    vga_print("\nDynamically allocating physical memory...\n");

    //asking for new pmm block
    uint32_t new_physical_block=pmm_alloc_block();
    //new mapper to link the crashed Virtual Address to the new block
    map_page(fault_address, new_physical_block);
    vga_print("Map fixed. Resuming execution.\n");

}


//automatic handling of the page fault exception 14 by kernel
uint32_t map_page(uint32_t virtual_address, uint32_t physical_address) {
    uint32_t directory_index=(virtual_address >> 22) & 0x3FF; //0x3FF because it is the hexadecimal representation of the binary number 1111111111, that isolates exactly 10 bits of data while wiping out everything else.
    uint32_t table_index=(virtual_address >> 12) & 0x3FF;
    if (!(kernel_directory->entries[directory_index]&1)) { //checks if Bit 0 is a 1
        //if 0
        uint32_t new_table_phy=pmm_alloc_block(); //bucket is empty! We must ask the PMM for a new physical block
        page_table* new_table=(page_table*)new_table_phy; //cast it to our blueprint so we can clear it out
        for (int i=0;i<1024;i++) { //clear the new Page Table
            new_table->entries[i] =0x02; //Not Present, Read/Write
        }
        kernel_directory->entries[directory_index]=new_table_phy | 3; //link the brand new Page Table into the Directory bucket
        //use | 3 to set Present and Read/Write flags
        uint32_t table_phys_addr = kernel_directory->entries[directory_index] & ~0xFFF; //extract the physical address of the Page Table from the Directory
        //We use '& ~0xFFF' to strip the 12 security flags off the bottom,
        // leaving only the pure physical address.
        page_table* table = (page_table*)table_phys_addr;
        //the target Physical Address into the correct Table slot!
        table->entries[table_index] = physical_address | 3;

    }


}







/* basic paging explanation.
imagine an apartment building (Physical RAM). It has physical addresses: Apartment 100, Apartment 101, Apartment 102.

if we write a program and hardcode it to run at "Apartment 100," it will work perfectly.
but what if we run two copies of that program? They both try to move into Apartment 100 at the same time. They fight, destroy each other's furniture (data), and the OS crashes.

Paging is the OS creating an illusion.
when we turn on Paging, the OS stops handing out real apartment numbers. Instead, it hands out Virtual Addresses.
the OS tells Program A: "You live in Apartment 100." (But secretly, the OS puts Program A in Physical Apartment 205).
the OS tells Program B: "You live in Apartment 100." (But secretly, the OS puts Program B in Physical Apartment 803).
both programs think they own "Apartment 100." They never fight, because they are mathematically separated. Paging is the mechanism that creates secure sandboxes for every program running on our computer.

Part 2: The Two-Tier Map (The Post Office)
to keep track of this massive illusion, the CPU needs a translation map. If Program A asks for "Virtual Apartment 100," the CPU needs to quickly look up where that is physically located.
because 4 Gigabytes of memory is massive, we can't use one giant list. We use a Two-Tier System, exactly like a Post Office routing a package.
the Page Directory (The State Level): This is a small list (1024 entries). It doesn't tell you the exact apartment; it just tells you which city the package goes to.
the Page Table (The City Level): This is another small list (1024 entries). Once you are in the right city, this list tells you the exact physical apartment number.
every single process on your computer (and your kernel itself) gets exactly one Page Directory (The Master Index).
 */

