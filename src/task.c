#include "include/task.h"
#include "include/heap.h"
#include "include/paging.h"
#include "include/vga.h"
#include "include/gdt.h"
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
    kernel_task->id = next_pid++;
    kernel_task->esp=0;//scheduler will fill this in automatically on the first tick check the timer_stub and timer.c for more details
    //kernel's virtual memory map is just the master directory
    extern page_directory* kernel_directory;
    kernel_task->page_dire = kernel_directory;

    extern tss_entry_t tss_entry;
    tss_entry.ss0 = 0x10; // Kernel Data Segment

    //it is the only task, so it points to itself
    kernel_task->next=kernel_task; //next right back to kernel_task to create the circular loop.
    current_task=kernel_task;
    ready_queue=kernel_task;
    klog_ok("Task Management Initialized");
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

/*
 * Dual Stack Architecture.
 * when a program runs in Ring 3 (User Mode), it has its own private User Stack for its local variables. But what happens when the hardware timer ticks and forces the CPU to jump to your Ring 0 Kernel to handle the interrupt?
 * the Kernel cannot use the User Stack to save its registers, because the Ring 3 program might have maliciously corrupted it.
 * therefore, every single Ring 3 program actually requires two stacks:
User Stack: Used while the program is running normally in Ring 3.
Kernel Stack: Hidden away in Ring 0. Used exclusively to safely catch the CPU's registers when an interrupt fires.
 */

void create_task(void (*entry_point)()) {
    //this is where the CPU will dump its registers when a hardware interrupt happens.
    //it must be completely inaccessible to ring 3 programs to prevent tampering.
    uint32_t* base_ptr_kernel_stack = (uint32_t*)kmalloc(1024);
    //move the pointer to the absolute top of the 1KB array (256 slots * 4 bytes = 1024 bytes)
    //stacks grow downwards, so we start at the top.
    uint32_t* kernel_stack = base_ptr_kernel_stack+256;
    //this is where the program will store its normal local variables (like 'int count = 5;').
    uint32_t* base_ptr_user_stack = (uint32_t*)kmalloc(1024);
    uint32_t* user_stack = base_ptr_user_stack + 256;
    //by default, kmalloc pulls memory from the Heap, which is locked to Ring 0 (Supervisor).
    //if the User Program tries to touch its own User Stack, the MMU will throw a Page Fault.
    //we strip the bottom 12 bits to find the 4KB aligned physical address, and use map_page
    // to explicitly unlock this specific chunk of memory for Ring 3.
    uint32_t aligned_stack_addr = (uint32_t)base_ptr_user_stack & ~0xFFF;
    map_page(aligned_stack_addr, aligned_stack_addr);
    //we are manually placing 5 specific numbers onto the KERNEL stack.
    //when the CPU executes the 'iret' (Interrupt Return) command later, it will violently
    //rip these 5 plates off the stack and use them to launch the program.
    kernel_stack--;
    *kernel_stack = 0x23; //PLATE 1: User Data Segment (SS)
    //tells the CPU: "Your data now belongs to Ring 3."

    kernel_stack--;
    *kernel_stack = (uint32_t)user_stack; //PLATE 2: User Stack Pointer (ESP)
    //tells the CPU: "Here is the address of the Ring 3 User Stack."

    kernel_stack--;
    *kernel_stack = 0x202; //PLATE 3: EFLAGS (Interrupts On)
    //the '2' in the middle ensures the Interrupt Flag (IF) is 1.
    //if we don't do this, the program will ignore the hardware timer and freeze the OS.

    kernel_stack--;
    *kernel_stack = 0x1B; //PLATE 4: User Code Segment (CS)
    //tells the CPU: "You are now executing code with Ring 3 restrictions."

    kernel_stack--;
    *kernel_stack = (uint32_t)entry_point; //PLATE 5: Instruction Pointer (EIP)
    //tells the CPU the exact memory address of `program_A` to start running.

    //our assembly interrupt stub uses the 'popa' instruction to restore CPU registers.
    // 'popa' blindly grabs the top 8 plates off the stack and shoves them into EAX, EBX, ECX, etc.
    //because this is a brand new program, those registers don't have any saved data yet.
    //if we don't put 8 fake plates here, 'popa' will accidentally eat our 5-Plate IRET frame!
    for (int i=0; i<8; i++) {
        kernel_stack--;
        *kernel_stack = 0; //push 8 zeros as "empty calories" for popa to eat.
    }
    //building the pcb
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    new_task->id = next_pid++;
    //we save the exact location of our forged Kernel Stack into the ledger.
    //when the OS wants to start this program, it will point the CPU at this exact address.
    new_task->esp = (uint32_t)kernel_stack;

    //assign the memory sandbox (currently sharing the kernel's sandbox)
    extern page_directory* kernel_directory;
    new_task->page_dire = kernel_directory;
    //we insert this new task into our circular linked list so the scheduler can find it.
    new_task->next = current_task->next;
    current_task->next = new_task;
}