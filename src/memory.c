#include "include/memory.h"
#include "include/multiboot.h"
#include "include/vga.h"
#include "include/pmm.h"

// we need to parse/read the map of RAM which we got from multiboot.h & BIOS
void parse_memory_map(void* mb_info_ptr) {
    //Casting the raw void pointer back into our multiboot struct type
    multiboot_info_t* mb_info = (multiboot_info_t*)mb_info_ptr;
    /*
    * You are creating a brand new pointer named mb_info.
    * You are taking the raw memory address (mb_info_ptr), and you are forcefully telling the compiler: "Treat this raw memory address as if it holds a multiboot_info_t structure."
    * Now, when you type mb_info->flags, the compiler knows exactly how many bytes to skip to find that specific variable.
     */


    //Security Check: Did GRUB actually provide a memory map?
    if (!(mb_info->flags&(1<<6))) { //Bit 6 of the flags variable must be set to 1
        vga_print("ERROR: GRUB did not provide a valid memory map!\n");
        return;
    }
    /*
    * When GRUB hands us the mb_info briefcase, it doesn't just hand us the memory map. It hands us a dozen different things (the boot device name, the kernel command line, video card info, etc.).
    * However, depending on the motherboard, GRUB might fail to get some of those things. To tell us what it successfully found, GRUB gives us a single 32-bit integer called flags.
    * Instead of giving us 32 different boolean variables (bool has_memory_map = true; bool has_video_info = false;), GRUB uses the 32 individual bits inside that one integer as tiny on/off switches.
    * According to the official Multiboot manual: Bit number 6 is the switch for the Memory Map.
    *   If Bit 6 is a 1, the memory map is in the briefcase.
    *   If Bit 6 is a 0, GRUB failed to get it, and we shouldn't look for it.
    *
    * 1 << 6 (Left Shift): This takes the binary number 1 and shifts it left by 6 places.
    * Starts as: 00000000 00000000 00000000 00000001, Becomes: 00000000 00000000 00000000 01000000 (This creates a "mask" where ONLY Bit 6 is turned on).
    * & (Bitwise AND): This compares our mask against the actual flags variable GRUB gave us. It looks at them stack-on-top-of-each-other. If both have a 1 in the 6th spot, the result is greater than zero (True). If GRUB has a 0 there, the result is zero (False).
    * ! (The NOT operator): We want to throw an error if the map is missing. So we say, "If the result of that bitwise check is NOT true, print the error."
     */


    vga_print("System Memory map:\n");
    uint32_t mmap_addr = mb_info->mmap_addr; //get the starting physical address of the map array
    uint32_t mmap_length = mb_info->mmap_length; //get the total size of the map array in bytes
    //our "reading finger" (mmap) at the starting line
    multiboot_mmap_t* mmap = (multiboot_mmap_t*)mmap_addr; //a pointer to track our current position in the array

    //Keep reading until our finger crosses the finish line
    while ((uint32_t)mmap < mmap_addr + mmap_length) { //loop that continues as long as the physical memory address of the 'mmap' pointer is strictly less than (map_addr + map_length).

        vga_print("Base: ");//the base address of the ram aka starting
        vga_print_hex_64(mmap->addr); //64bit add
        vga_print(" | Length: "); //the length of the ram
        vga_print_hex_64(mmap->len);

        //type
        if (mmap->type == 1) {
            vga_print(" | TYPE: AVAILABLE\n");
            uint64_t start_address = mmap->addr;//where does this chunk of available RAM start?
            uint64_t end_address = mmap->addr + mmap->len; //where does this chunk end? (Start + Length)
            for (uint64_t current_addr = start_address; current_addr < end_address; current_addr += 4096) {
                pmm_free_memory( (uint32_t)current_addr );
            }
        }else {
            vga_print(" | TYPE: RESERVED\n");
        }
        mmap = (multiboot_mmap_t*)((uint32_t)mmap + mmap->size+4);
        /*
        * move the 'mmap' pointer forward to the next entry in the array. GRUB memory maps are NOT standard C arrays.
        * must take the raw integer memory address of our current pointer, and mathematically add (mmap->size + 4) to it.
        *
        * Why can't we just write mmap++?
        * If we have an array of integers in C, int arr[5], and we type arr++, the compiler moves forward exactly 4 bytes, because every int is identically 4 bytes.
        * But the GRUB memory map is messy. The BIOS might report the first RAM chip using 20 bytes of data. But the second RAM chip might have special power-management features, so the BIOS uses 24 bytes of data to describe it.
        *
        * If we tell the C compiler to just do mmap++, it rigidly assumes every entry is 24 bytes (the size of our multiboot_mmap_t struct).
        * When reading the first entry (which is only 20 bytes physically), mmap++ makes the compiler jump 24 bytes forward. It overshoots the start of the next entry by 4 bytes.
        * We read garbage data, calculate a massive wrong jump next time, and crash the OS.
        *
        * So, we manually: ((uint32_t)mmap): We tell C, "Stop thinking of this as a rigid struct. Treat my reading finger as a raw number."
        * + mmap->size: We look at the actual physical size reported by the hardware for this specific entry and add it. + 4: We manually skip over the 4-byte size variable itself.
        * (multiboot_mmap_t*)(...): We turn it back into a pointer so we can read the next block.
        */
    }

}