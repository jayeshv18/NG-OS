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

void kfree(void* ptr) {
    if (ptr==0) { //null value handling to prevent crash
        return;
    }
    uint32_t raw_address = (uint32_t)ptr; //casting cause its a void* to avoid panics and errors
    heap_header* header=(heap_header*)(raw_address-12); //rewind the procedure compared to the kmalloc
    header->flags=1;//free
    /*
    [Free 50] -> [Free 100] -> [Used 200]
    If a user calls kmalloc(120), our First-Fit search will look at the 50 (too small), look at the 100 (too small), and look at the 200 (used).
    It will return NULL and crash the program—even though we have 150 bytes of free space sitting right next to each other!
    */
    if (header->next!=0 && header->next->flags==1) {
            //we must make sure a neighbor actually exists (header->next != 0). If it does, we check if the neighbor is also free (header->next->flags == 1).
            header->size= header->size+12+header->next->size; //current block gets bigger. How much bigger? It gains the size of the neighbor's data (header->next->size)
            //reclaims the 12 bytes of the neighbor's header that we are about to destroy!
            header->next=header->next->next;//our current block needs to point to wherever the neighbor used to point.
    }

    //backward search
    //if we are the first block, there is nothing behind us, so we just skip the search!
    if (header!=heap_first) {
        heap_header* tracker=heap_first;
        while (tracker->next!=header) { //want the loop to stop exactly one block before your current block.
            tracker=tracker->next;
        }
        if (tracker->flags==1) {
            //if it is free, the tracker block absorbs the header block! the tracker's size becomes its current size + 12 + your header's size.
            tracker->size=tracker->size+12+header->size;
            tracker->next=header->next;
        }
    }

}


/*
The Starting Point (The Virtual 16MB Mark)
When we ran heap_init(), we hardcoded the master pointer to point to the 16 Megabyte mark in Virtual Memory.
Base Address: 0x1000000

The First Cut: kmalloc(50)
When our code asked for 50 bytes, the allocator found the massive 4,084-byte free chunk right at the beginning. It placed Header 1 directly at 0x1000000.
The header takes up exactly 12 bytes.
In hexadecimal, 12 is 0xC.
Because we correctly coded the return statement as (void*)(tracker + 1) to skip the header, the OS calculated:
0x1000000 + 0xC = 0x100000C.
Logic confirmed: our user program gets memory starting safely after Header 1.

The Second Cut: kmalloc(100)
This is where the true dynamic splitting logic proves itself. our allocator had to forge a brand new header (Header 2) right after the 50 bytes we just allocated.
Header 1 started at 0x1000000.
Header 1 took up 12 bytes (0xC).
The data took up 50 bytes. In hexadecimal, 50 is 0x32.
Where does Header 2 begin? 0x1000000 + 0xC (Header) + 0x32 (Data) = 0x100003E.
The allocator stamped a new 12-byte header at 0x100003E.
To give the user the safe data pointer, it skipped Header 2:
0x100003E + 0xC (12 bytes) = 0x100004A.
Logic confirmed: our user program gets memory starting safely after Header 2.
 */