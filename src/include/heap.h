/*
* The Kernel Heap is a secondary layer of memory management that sits on top of our PMM and VMM.
Instead of talking to the hardware, the Heap is essentially a "butcher."
    The Heap asks the VMM for a raw, 4KB block of memory (a whole carcass).
    When our C code asks for 15 bytes, the Heap takes its meat cleaver, slices exactly 15 bytes off that 4KB block, and hands it to us.
    The Heap keeps track of the remaining 4,081 bytes so it can hand them to the next program that asks.

When we call free(ptr), how does the OS know how many bytes to free? we only gave it a memory address; we didn't give it a size!
It is not magic. It is an architectural trick called Inline Metadata.

Whenever the Heap hands us memory, it secretly steals a few bytes right before the memory address it gives us.
It uses this stolen space to place a hidden C struct (a Header) containing the management data.

The Linked List of Blocks
The Heap manages memory as a massive Linked List. Every single chunk of memory in our OS will begin with a Header. That Header will contain:
    The size of this specific chunk.
    A flag saying if it is 0 (Free) or 1 (Used).
    A pointer to the exact memory address where the next Header begins.

When a program asks for memory (e.g., kmalloc(50)), the Heap Engine performs the following algorithm:
    The Search (First-Fit): It starts at the very first Header in the heap. It checks the flag: Is this chunk free? If yes, it checks the size: Is this chunk >= 50 bytes?
    If no, it uses the next pointer to jump to the next Header and checks again.

    The Split: Once it finds a chunk that is big enough (like our brand new 4096-byte block), it doesn't just give the whole thing away! It calculates the math to split the block.
        It modifies the current Header to say size = 50 and is_free = 0.
        It calculates the exact memory address where those 50 bytes end, and it forcibly writes a brand new Header into the RAM at that exact spot.
        It sets the new Header to is_free = 1, calculates the remaining leftover size, and links the two Headers together.

    The Return: It takes the memory address of the first Header, adds the size of the Header to it (to skip over the metadata), and returns that pure data address to the C program.

 */

#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>

typedef struct header_name {
    uint32_t size; //need to track exactly how large this specific chunk of memory is
    uint32_t flags; //need a way to know if this chunk is 0 (Used) or 1 (Free).
    struct header_name *next; //header needs a pointer that points to the exact memory address of the next header in the physical RAM.
//the next pointer needs to point to another header of the exact same type, we run into a classic C compilation trap. We cannot point to a typedef before the typedef is finished compiling!
//so we use struct header_name
}heap_header;
void heap_init();
void* kmalloc(uint32_t requested_size);
void kfree(void* ptr);
#endif
