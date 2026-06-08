#include "include/task.h"
#include "include/heap.h"
#include "include/paging.h"
task_t* current_task; //who is running right now?
task_t* ready_queue; //who is waiting in line?
uint32_t next_pid=1; //autoincrementing ID for new programs
/*
 logic click:
task_t struct actually has three variables,
uint32_t id; (4 bytes)
uint32_t esp; (4 bytes)
struct task* next; (A pointer is just a 32-bit memory address, so it is also 4 bytes). Because it has three 32-bit variables, a single task_t struct requires exactly 12 bytes of physical RAM to exist.

when we call kmalloc(sizeof(task_t)), we are walking up to the Heap Manager we built and saying:
i need you to find me a block of 12 contiguous bytes of free RAM, lock them down so no one else can overwrite them, and tell me where they are.
kmalloc scans the heap, finds the space, and returns a Pointer (a raw memory address).

Let's pretend kmalloc finds free space starting at memory address 0x0100000.
kmalloc returns the number 0x0100000 to us. That is it. That number is assigned to our kernel_task variable.

the compiler knows that kernel_task is pointing to address 0x0100000, and it knows that a task_t struct is basically a 12-byte map.
it automatically calculates the "offsets" for our variables so they sit perfectly inside those 12 bytes on the Heap:
    The id variable sits at the very beginning: 0x0100000. (It takes up bytes 0, 1, 2, and 3).
    The esp variable sits right after it: 0x0100004. (It takes up bytes 4, 5, 6, and 7).
    The next variable sits at the end: 0x0100008. (It takes up bytes 8, 9, 10, and 11).

when we write kernel_task->esp = 0; in our C code, the CPU doesn't know what "esp" means.
the compiler translates that into assembly language that says: "Go to memory address 0x0100000, add 4 bytes to it, and write the number 0 right there.

kernel_task is just a local variable holding a piece of paper with an address written on it (0x0100000).
the actual struct data (the ID, the ESP, the Next pointer) physically lives out on the Heap at that address.
*/

void task_init() {
    task_t* kernel_task=(task_t*)kmalloc(sizeof(task_t)); //Dynamic Allocation for memory using kmalloc.
    kernel_task->esp=0;
    kernel_task->id = next_pid;
    next_pid++;
    kernel_task->next=kernel_task; //next right back to kernel_task to create the circular loop.
    current_task=kernel_task;
    ready_queue=kernel_task;

}

/*
 * the code had to be updated due to miscalculation but that teaches an important concept
 *
 * heap_init():
   heap_first->size=4084; // full block is 4096. 4096 - 12 = 4084 bytes free.
   earlier: create_task() (in task.c):
   kmalloc((4096)); // ask the heap for 4096 bytes

    4,084 bytes of free memory in our entire operating system.
    task A walks up to the Heap Manager and asks for 4,096 bytes.
    because 4096 is larger than 4084, our if(tracker->size >= requested_size) statement evaluates to false. The while loop finishes, and kmalloc correctly executes its final safety line: return 0;.
    kmalloc hands Task A a NULL pointer (0x0).
    a millisecond later, Task B asks for 4096 bytes. kmalloc hands Task B a NULL pointer (0x0).

    in create_task, i wrote:
    uint32_t* stack = base_ptr_stack + 1024;
    because base_ptr_stack was 0x0, the compiler did this math:
    0x0 + (1024 slots * 4 bytes) = 0x1000.
    task A went to physical memory address 0x1000 and forged its stack.
    task B did the exact same math, went to 0x1000, crushed Task A, and forged its stack.
    the Shared Stack Collision was caused because both programs got rejected by the Heap Manager!

heap is only 1 page long (4KB). That is a tiny heap, but it is more than enough to run these two little test programs. We just need to give them smaller stacks.
allocation from 4096 bytes to 1024 bytes (1KB).
stack array is now only 1024 bytes long, you cannot advance the pointer by 1024 slots anymore! 1024 bytes divided by 4 bytes per slot = 256 slots.

 */

void create_task(void (*entry_point)()) {
    uint32_t* base_ptr_stack=(uint32_t*)kmalloc((1024)); //heap for 1024 bytes and cast it to a uint32_t*
    //pointer is currently sitting at the bottom of the 4KB block (e.g., byte 0). Stacks grow downward from the top.
    uint32_t* stack=base_ptr_stack+256; //stack is a 32-bit pointer (4 bytes per slot), 1024 bytes equals exactly 256 slots. We need to move our pointer to the absolute end of the array. Advance our stack pointer by exactly 256 slots.
    stack--;
    *stack=0x202; /*EFLAGS register is the CPU's master status panel. It tracks things like "did the last math operation equal zero?"
    bit 9 of this register is the Interrupt Enable Flag (IF). If Bit 9 is a 1, the CPU listens to the PIT alarm clock. If Bit 9 is a 0, the CPU puts on noise-canceling headphones and ignores all hardware.
    0x202 in binary is 0000 0010 0000 0010, 9th bit is 1.
    If we forget to push 0x202 and accidentally push 0x000, our new program will wake up, put on the noise-canceling headphones,
    ignore the PIT alarm clock, and our OS will permanently freeze. 0x202 guarantees the program can be paused later.
    */
    stack--;
    *stack=0x08; /*think about the GDT structure.
    *each GDT entry is exactly 8 bytes long, Entry 1 sits at memory address 8 (which is 0x08 in hex). By pushing 0x08 onto the stack,
    *we are telling the CPU's memory management unit: "When you wake up this program, run it with the security clearance defined in GDT Entry 1."
    * Entry 1 was our Kernel Code Segment (Ring 0, full root access).
     */
    stack--;
    *stack=(uint32_t)entry_point;
    /*
    inside the CPU, the EIP register (Extended Instruction Pointer) physically aims at the exact next line of code the CPU needs to run.
    when we pass a C function into our task creator (like void my_program()), entry_point holds the physical memory address of where my_program lives in RAM.
    by putting this at the bottom of the iret frame, the iret instruction physically yanks this address off the stack, jams it into EIP, and the CPU instantly begins executing our C function.
     */

    /*
    * Remember our Assembly stub? Right before it calls iret, it calls popa.
    popa is violently hungry. It is hardwired to reach into the stack and rip out exactly 8 chunks of 32-bit data to fill the general registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP, and a throwaway ESP).
    if we don't feed popa 8 chunks of data, it will keep digging into the stack and accidentally eat our EIP, our CS, and our EFLAGS!
    by planting 8 zeros on the stack, we are just feeding popa empty calories so it gets full. It eats the zeros, leaves the vital iret frame untouched, and the context switch completes perfectly.
    TRY TO READ ALL COMMENTS BECAUSE IT IS THERE FOR A REASON :|
    */
    for (int i=0;i<8;i++) {
        stack--;
        *stack=0;
    }
    task_t* new_task=(task_t*)kmalloc(sizeof(task_t));
    new_task->id=next_pid;
    next_pid++;
    new_task->esp=(uint32_t)stack;
    //circular link
    new_task->next=current_task->next;
    current_task->next=new_task;
};