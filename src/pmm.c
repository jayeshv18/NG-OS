#include "include/pmm.h"
#include <stdint.h>
static uint32_t total_blocks=0; //total slots on motherboard
static uint32_t used_blocks=0; //number of blocks that are currently in use
static uint32_t* pmm_bitmap=0; // 32 bit ptr to bitmap address
/*
* When we compile C code, the compiler just turns text into binary instructions. It has no idea where those instructions should live in RAM. It is the Linker's job to take all our kernel.o, vga.o, and pmm.o files and glue them together into the final ng-os.iso.
* Because the Linker physically builds the final file, the Linker knows exactly how many bytes long our entire operating system is.
* If we open our linker.ld file, we will usually see a command at the very bottom that looks like this: _kernel_end = .;
* That little dot (.) means "Current Address." The Linker is saying, "Hey, I just finished placing the very last byte of code for this OS. Let me mark down this exact memory address and call it _kernel_end."
 */
void pmm_init(uint32_t kernel_end_addr, uint32_t total_ram_size) {
    total_blocks = total_ram_size / BLOCK_SIZE; //total_ram_size (in bytes). Divide that by our BLOCK_SIZE to find out exactly how many 4KB blocks exist on the motherboard
    uint32_t bitmap_array_length=(total_blocks/32); //one uint32_t (4 bytes) tracks 32 blocks, this is to find out how many bytes the map itself takes.
    //want the raw number. We have to cast the raw number into a pointer
    pmm_bitmap=(uint32_t*)kernel_end_addr; //the safe place to store bitmap data and protect it from overwriting ie the linker says our kernel ends at 0x105000, that means 0x105001 is guaranteed to be empty, usable space. That is exactly where we will place our Bitmap pointer.
    //lock down the entire array by writing 1s.
    for (uint32_t i=0; i<bitmap_array_length; i++) {
        /*
         * We need to lock down the entire array. But we aren't flipping bits individually here; we can be fast.
         * Since we want every single bit to be a 1 (Used), we can just write the maximum 32-bit integer (0xFFFFFFFF) into every slot of the array.
         */
        pmm_bitmap[i]=0xFFFFFFFF;
    }
    //at startup, all blocks are technically "used" (locked down).
    used_blocks = total_blocks;
}

uint32_t pmm_get_used_blocks() {
    return used_blocks;
}
uint32_t pmm_get_total_blocks() {
    return total_blocks;
}

void pmm_free_memory(uint32_t physical_address) {

    /*
     * by shifting a 1 into position, inverting it so that position becomes a 0 (and everything else is 1),
     * and then applying an AND mask, we successfully blow a single hole in the lockdown without touching any adjacent blocks.
     *
    * when kernel.c eventually calls this function, it isn't going to say: "Free bit 5 inside array index 2.
    * "it is going to say: "I found a block of available RAM starting at 0x100000. Free it."
    * we need to translate a physical memory address (0x100000) into two specific numbers:
    *   block number (i): Which of the 16,384 integers holds the bit we want? eg: consider 2GB ram.
    *   bit position (bit_position): Which of the 32 bits inside that specific integer do we flip?
     */
    uint32_t block_index = physical_address / BLOCK_SIZE; // block_index already refers to an address we just need to convert that physical address, if we divide 0x100000 by 4096, we find out this is the 256th block on the motherboard.
    // Prevent the OS from freeing memory that physically does not exist.
    if (block_index >= total_blocks) {
        return; // Abort immediately
    }
    uint32_t i=block_index/32; //block 256 divided by 32 means it lives in the 8th integer of our array ie pmm_bitmap[8]
    uint32_t bit_position=block_index%32; //modulus operator gives us the remainder. 256 divided by 32 has a remainder of 0, meaning it is the very first bit of that 8th integer. ie bit 1 of pmm_bitmap[8].
    /*
     * Check if the bit is ALREADY a 0.
     * We use the bitwise AND (&) to isolate the bit. If the result is 0,
     * it means the block is already free. We abort to prevent underflowing
     * the used_blocks counter.
     */
    if ((pmm_bitmap[i] & (1 << bit_position)) == 0) {
        return; // Already free, abort immediately
    }
    //survived the guards, it is safe to free the block.
    pmm_bitmap[i]&=~(1<<bit_position); //bitwise math to flip the 1 to 0.
    used_blocks--;
}

/*
 * before we can hand out a block of memory, we need a hidden, internal helper function that searches the ledger and returns the absolute block index of the very first free block it can find.
* pmm_bitmap is an array of 32-bit integers. If an integer is exactly equal to 0xFFFFFFFF, it means every single one of its 32 bits is a 1 (all 32 blocks are currently in use).
*  read pmm_bitmap[0]. Is it 0xFFFFFFFF? Yes. Skip it.
*  read pmm_bitmap[1]. Is it 0xFFFFFFFF? Yes. Skip it.
*  read pmm_bitmap[2]. Is it 0xEFFFFFFF? Wait, that isn't full! There is at least one 0 hiding inside this integer.
*  now we slow down and use a tiny inner loop (0 to 31) to test the bits of this specific integer to find exactly which one is the 0.
 */

uint32_t pmm_first_free() {
    for (uint32_t i=0; i<total_blocks/32; i++) { //iterates through the integers in our pmm_bitmap array (from 0 to total_blocks / 32).
        if (pmm_bitmap[i] != 0xFFFFFFFF) { //check if the current integer is NOT equal to 0xFFFFFFFF
            for (int j=0; j<32; j++) { //iterates through bits 0 to 31.
                if ((pmm_bitmap[i] & (1 << j))==0) { //bitwise test (pmm_bitmap[i] & (1 << j)) == 0 to find the exact bit that is free.
                    return (i*32)+j; //convert the array index (i) and the bit position (j) back into the absolute block number and return it.
                }
            }
        }
    }
    return 0xFFFFFFFF;
}

//public function that kernel can call
uint32_t pmm_alloc_block() { //this is malloc equivalent for our kernel
    uint32_t index=pmm_first_free(); //to get the index of the free block.
    if (index == 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }
    pmm_bitmap[index/32]|=(1<<(index%32));
    used_blocks++; //update the used_blocks dashboard counter.
    return index*BLOCK_SIZE; //convert that block number back into a physical RAM address (e.g., 0x105000) and return it to the caller
}


//manually lock a specific physical address (flip it to 1)
/*
 *Allocated Block 1 at: 0x0000000000000000 this results ina security bug.
 * in C, a pointer to 0x0 is called the NULL pointer. If a C program calls malloc() and receives 0x0, the program automatically assumes it means "Out of Memory" and will intentionally crash!
 * address 0x0 is the very first byte of RAM on the motherboard. Even though GRUB told us it was "AVAILABLE", that specific memory block contains the legacy Real Mode Interrupt Vector Table (IVT).
 * if we hand that to a user program and they overwrite it, our motherboard might act unpredictably.
 *
 * our pmm_init locked everything. But then our parse_memory_map scanned the GRUB map, saw that the first chunk of RAM (0x0 to 0x9FC00) was marked as 1 (Available), and blindly unlocked it!
 * we lock it like pmm_lock_memory(0x0);
 */
void pmm_lock_memory(uint32_t physical_address) {
    uint32_t block_index = physical_address / BLOCK_SIZE;
    uint32_t i = block_index / 32;
    uint32_t bit_position = block_index % 32;

    // Bitwise OR to force it to 1
    pmm_bitmap[i] |= (1 << bit_position);
    used_blocks++;
}