#include "include/heap.h"

#include <stddef.h>

#include "include/paging.h"
#include "include/pmm.h"
/*
 * Before we can start carving up memory for kmalloc, we have to jumpstart the heap.
 * The heap needs a starting location in Virtual Memory, and it needs a Master Pointer so the OS always knows where the very first header is located.
 */

heap_header* heap_first=0; //need a global pointer to track the start of our linked list.
void heap_init() {
    uint32_t phys_block=pmm_alloc_block(); //PMM for a new physical block
    map_page(0x1000000,phys_block);//need to pick a Virtual Address where the heap will live. Let's pick the 16 Megabyte mark: 0x1000000. This is far away from our kernel code (which is at 1MB), so they won't collide.
    heap_first=(heap_header*)0x1000000; //casting virtual address to the heap
    /*
    * We could have picked almost any virtual address for our heap to start at. But we chose 0x1000000 (which is exactly 16 Megabytes) for a specific architectural reason:
    * our kernel code lives at the 1 Megabyte mark (0x100000). If we put the heap too close to the kernel, and the heap grows, it might accidentally overwrite our kernel code.
    * by placing the heap at the 16MB mark in Virtual Memory, we give the kernel 15 Megabytes of empty space to breathe.
     */
    //full block is 4096 bytes. Our header takes up 12 bytes. Therefore, the usable free space inside this block is exactly 4096 - 12 = 4084 bytes.
    heap_first->size=4084;
    heap_first->flags = 1; //Mark it as Free
    heap_first->next = 0; //There are no other headers yet, so it points to NULL
}
/*
 * When a program calls kmalloc(50), we don't just blindly chop memory. We have to search our Linked List to find a block that is both Free and Big Enough. We call this the First-Fit Search Algorithm.
 */
void* kmalloc(uint32_t requested_size) {
    heap_header* tracker=heap_first; //temporary "tracker" pointer and point it at the start of the heap
    while (tracker!=NULL) { //loop until we run off memory
        if(tracker->flags==1 && tracker->size>=requested_size) { //if its free and big enough then return
            int leftover_size=tracker->size-requested_size; //use an integer in case the math goes negative
            if(leftover_size>0) {
                uint32_t raw_address=(uint32_t)tracker; //Convert the pointer to a raw integer so we can do exact byte math
                uint32_t new_address=raw_address+12+requested_size; //Calculate where the new header goes: Current + 12 (Header) + Requested Size
                heap_header* new_header=(heap_header*)new_address; //Cast that raw address back into a header blueprint!

                //Initialize the new header
                new_header->size = leftover_size;
                new_header->flags = 1; // It is free
                new_header->next = tracker->next; // Link it to the rest of the chain

                //Update the old header (tracker)
                tracker->size = requested_size;
                tracker->next = new_header; // Link it to our new split block
            }

            //mark the current block as used
            tracker->flags=0;
            //Return the usable memory address (skipping the 12-byte header)
            return (void*)(tracker+1);
            //tracker is a typed pointer, if we write tracker + 1, the compiler doesn't add 1 byte. It adds exactly 1 "chunk" of whatever the pointer type is (in this case, it mathematically adds 12 bytes to skip the header).
        }
        tracker=tracker->next;//else go to next
    }
    //If the while loop finishes and we get here, the heap is completely full!
    return 0;
}