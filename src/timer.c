#include <stddef.h>
#include <stdint.h>
#include "include/io.h"
#include "include/vga.h"
#include "include/task.h"
#include "include/gdt.h"
uint32_t tick=0; //tracks total system uptime.
void timer_handler_main(uint32_t* current_esp_ptr) {
    tick++;
    /*Code-Update until the next ----> this code was writtten to check the asynchronous multiptasking between the timer and keyboard.
    if (tick%18==0) {
        vga_print(".");
    }
    ----->
    */
    outb(0x20, 0x20); //End of Interrupt to the Master PIC command port
//OS still needs to count the ticks internally; it just shouldn't spam the screen.

    //deferencing
    uint32_t current_esp = *current_esp_ptr;
    if (current_task==NULL) {
        //the OS is still booting and multitasking hasn't started yet.
        return;
    }
    current_task->esp=current_esp;
        //if we don't save current_esp somewhere permanent, the exact moment we switch to program_B, we will completely lose track of where program_A's snapshot is located in RAM. program_A would be lost in the void forever.
    current_task=current_task->next;
    if (current_task->esp == 0) {
            //if the task we just switched to has a 0 stack pointer, DO NOT RETURN 0. Return the stack we just came from.
        return ;
    }

        //we take the new task's current Stack Pointer.
        //we use & ~0x3FF (which is ~1023) to round DOWN to the start of the 1KB kmalloc block.
        //we add 1024 (0x400) to find the absolute TOP of that block.
        //we hand that address to the TSS!
        uint32_t stack_bottom = current_task->esp & ~0x3FF;
        uint32_t stack_top = stack_bottom + 1024;
        tss_entry.esp0 = stack_top;
        *current_esp_ptr = current_task->esp;
    }