;the C compiler assigns active kernel variables to physical CPU registers like EAX, EBX, ECX, and EDX to maximize execution speed, it assumes no outside force will alter them mid-sentence
; however, an unpredictable hardware interrupt instantly freezes this main execution loop (for instance, when EAX holds a critical math value like 42) and jumps directly to a C keyboard handler.
;Once inside the C function, the compiler freely overwrites those same registers with new data (such as changing EAX to 15 to read the port),
;meaning that when the handler finishes and the main loop resumes, its original register state is permanently corrupted, causing immediate system crashes or General Protection Faults.
;To prevent this, you must map the IDT entry to a tiny assembly wrapper stub that acts as a high-speed shield:
;it executes a pusha (or a manual push sequence in 64-bit mode) to take a snapshot of all general-purpose registers (EAX, EBX, ECX, EDX, ESP, EBP, ESI, EDI) and store them safely on the stack,
;calls the C function keyboard_handler_main() where the compiler can overwrite registers without consequence, executes popa to fully restore the original values back into their exact registers,
;and finally issues the iret (interrupt return) instruction to cleanly resume the pristine main kernel loop.

section .text
extern timer_handler_main
extern keyboard_handler_main
extern page_fault_handler
global timer_handler
global keyboard_handler ;need to make our assembly stub (function) available to our C code (for the IDT setup)
global isr14
bits 32

keyboard_handler:
pushad ;save ALL general registers (eax, ebx, ecx, etc.) onto the stack
call keyboard_handler_main ;run C code now! The compiler can overwrite whatever it wants
popad ;restore the exact original values back into the registers
;In 32-bit Protected Mode, both work, but popad is the technically more accurate
;popa (Pop All): This is the 16-bit instruction. It pops 8 words (16 bytes total) off the stack into the 16-bit registers (ax, cx, dx, bx, sp, bp, si, di).
;popad (Pop All Double-words): This is the 32-bit instruction. It pops 8 double-words (32 bytes total) off the stack into the modern 32-bit registers (eax, ecx, edx, ebx, esp, ebp, esi, edi).
iret ;safely resume the main loop using the specialized interrupt return

;if we dont make a copy of all registers then the C code might overwrite the registers and corrupt the data.

timer_handler:
pushad ;save the CPU state
call timer_handler_main ;run our C code)
popad ;restore the CPU state
iret ;return from the interrupt

global gdt_flush
gdt_flush:
mov eax, [esp+4]  ; Get the pointer passed from the C function
lgdt [eax]        ; Load the new GDT into the CPU

mov ax, 0x10      ; 0x10 is the Data Segment (Index 2)
mov ds, ax        ; Reload all data segment registers
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
jmp 0x08:.flush   ; Force a Far Jump to the Code Segment (0x08)
.flush:
ret               ; Return cleanly back to C

;page fault interrupt handle
extern page_fault_handler
global isr14
isr14:
;normal interrupts (like our keyboard driver), the CPU simply jumps to the code.
;but for a Page Fault, Intel's architecture automatically pushes a 32-bit Error Code onto the stack before it jumps to our handler.
pushad ;copy of the register
call page_fault_handler ;jump to the c logic
popad ; restore the cpu regs
add esp,4 ;extended stack pointer that always points to the absolute top.
;when the CPU puts a new plate on the stack (push), the memory address gets smaller (ESP goes down). When the CPU takes a plate off the stack (pop), the memory address gets larger (ESP goes up).
;when a normal hardware interrupt happens (like our keyboard), the CPU takes a snapshot of exactly where it was in our kernel code, pushes that return address onto the stack, and jumps to our handler.
;when we call iret (Interrupt Return), the CPU pops that return address off the stack and goes back to work.

;but Exception 14 (The Page Fault) is special.
;because it's a fatal error, the Intel CPU is designed to push one extra onto the stack right before it jumps to your handler.
;this extra plate is a 32-bit (4-byte) Error Code that contains technical details about why the page fault happened.

;if we call iret right now, the CPU will reach for the top plate to find the Return Address. But the top plate is currently the Error Code! The CPU will read the error code, assume it is a memory address, jump to it, and instantly Triple Fault.
;we must remove the Error Code plate from the stack. We could use pop eax to pull it off, but we don't actually care about the error code right now, and we don't want to corrupt the eax register.
;we manually manipulate the laser pointer.By executing add esp, 4, we are mathematically adding 4 bytes to the Stack Pointer. Because the stack grows upside down, adding 4 moves the pointer "down" the stack, perfectly skipping over the 4-byte Error Code. We abandon the data, realigning the stack perfectly so iret can grab the real Return Address.
iret





















