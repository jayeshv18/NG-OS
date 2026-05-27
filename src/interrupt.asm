;the C compiler assigns active kernel variables to physical CPU registers like EAX, EBX, ECX, and EDX to maximize execution speed, it assumes no outside force will alter them mid-sentence
; however, an unpredictable hardware interrupt instantly freezes this main execution loop (for instance, when EAX holds a critical math value like 42) and jumps directly to a C keyboard handler.
;Once inside the C function, the compiler freely overwrites those same registers with new data (such as changing EAX to 15 to read the port),
;meaning that when the handler finishes and the main loop resumes, its original register state is permanently corrupted, causing immediate system crashes or General Protection Faults.
;To prevent this, you must map the IDT entry to a tiny assembly wrapper stub that acts as a high-speed shield:
;it executes a pusha (or a manual push sequence in 64-bit mode) to take a snapshot of all general-purpose registers (EAX, EBX, ECX, EDX, ESP, EBP, ESI, EDI) and store them safely on the stack,
;calls the C function keyboard_handler_main() where the compiler can overwrite registers without consequence, executes popa to fully restore the original values back into their exact registers,
;and finally issues the iret (interrupt return) instruction to cleanly resume the pristine main kernel loop.

section .text
extern keyboard_handler_main
global keyboard_handler ;need to make our assembly stub (function) available to our C code (for the IDT setup)
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




























