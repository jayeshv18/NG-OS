#include <stddef.h>
#include <stdint.h>
#include "include/io.h"
#include "include/vga.h"
#include "include/task.h"
#include "include/gdt.h"
#include "include/paging.h"

//timer ensures the OS maintains the control over the CPU and  not trap in infinite loop and still multitask
uint32_t tick=0; //tracks total system uptime.
void timer_handler_main(uint32_t* current_esp) {
    tick++;
    /*Code-Update until the next ----> this code was writtten to check the asynchronous multiptasking between the timer and keyboard.
    if (tick%18==0) {
        vga_print(".");
    }
    ----->
    */

    //ALWAYS send EOI first! If we don't, the hardware permanently locks up.
    outb(0x20, 0x20); //End of Interrupt to the Master PIC command port
//OS still needs to count the ticks internally; it just shouldn't spam the screen.

    if (current_task==NULL) { //we ready to multitask?
        //the OS is still booting and multitasking hasn't started yet.
        return current_task;
    }
    //save the snapshot of the current program
    current_task->esp=current_esp;
        //if we don't save current_esp somewhere permanent, the exact moment we switch to program_B, we will completely lose track of where program_A's snapshot is located in RAM. program_A would be lost in the void forever.
    //rotate the queue
    current_task=current_task->next;
    if (current_task->esp == 0) {
            //if the task we just switched to has a 0 stack pointer, DO NOT RETURN 0. Return the stack we just came from.
        return current_esp;
    }
    //tell the MMU to swap out the memory
    switch_page_directory(current_task->page_dire);
        //we take the new task's current Stack Pointer.
        //we use & ~0x3FF (which is ~1023) to round DOWN to the start of the 1KB kmalloc block.
        //we add 1024 (0x400) to find the absolute TOP of that block.
        //we hand that address to the TSS!
    if (current_task->kernel_stack_top != 0) {
        tss_entry.esp0 = current_task->kernel_stack_top;
    }
    //return the new stack pointer via EAX register
    return current_task->esp;
    }