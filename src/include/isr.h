#ifndef ISR_H
#define ISR_H
#include <stdint.h>

//this struct MUST perfectly match the order of pushes in interrupts.asm
typedef struct registers {
    uint32_t ds;                                     //pushed manually by us
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pusha
    uint32_t int_no, err_code;                       //pushed by isr128
    uint32_t eip, cs, eflags, useresp, ss;           //pushed automatically by the CPU
} registers_t;

void isr_handler(registers_t* regs);

#endif