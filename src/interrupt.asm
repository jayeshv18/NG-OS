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
;because it's a fatal error, the Intel CPU is designed to push one extra onto the stack right before it jumps to our handler.
;this extra plate is a 32-bit (4-byte) Error Code that contains technical details about why the page fault happened.

;if we call iret right now, the CPU will reach for the top plate to find the Return Address. But the top plate is currently the Error Code! The CPU will read the error code, assume it is a memory address, jump to it, and instantly Triple Fault.
;we must remove the Error Code plate from the stack. We could use pop eax to pull it off, but we don't actually care about the error code right now, and we don't want to corrupt the eax register.
;we manually manipulate the laser pointer.By executing add esp, 4, we are mathematically adding 4 bytes to the Stack Pointer. Because the stack grows upside down, adding 4 moves the pointer "down" the stack, perfectly skipping over the 4-byte Error Code. We abandon the data, realigning the stack perfectly so iret can grab the real Return Address.
iret

global timer_stub
extern timer_handler_main
;program_A is running. The PIT timer ticks. The CPU violently pauses program_A and jumps to timer_stub.
;timer_stub executes pusha. At this exact microsecond, the entire state of program_A (its math, its code position, its flags) is perfectly frozen on the Kernel Stack.
;timer_stub pushes esp to pass the location of that snapshot to C.
;inside our C code, that location arrives as the variable current_esp.
timer_stub:
    pusha;dsave all general-purpose registers
    push esp ;onto the stack, this goes th=o the C function as the parameter
    call timer_handler_main ;call our C function
    ;when C finishes, the new task's ESP is waiting perfectly inside EAX
    mov esp, eax;HARDWARE STACK SWITCH! We are now on the new stack.
    ;in standard C convention, functions always return their answers inside the EAX register. Therefore, the brand new stack pointer we asked for is currently sitting in EAX.
    popa;restore all registers of new task
    iret;return from interrupt (crucial!)

;privelage drop
;need to take two parameters from C: the entry_point (where to jump) and the user_esp (where the new stack is).
;we are writing a C-callable assembly function, the parameters are waiting on the current kernel stack at [ebp+8] and [ebp+12].
global jump_to_usermode
jump_to_usermode:
push ebp
mov ebp,esp
;push ebp followed by mov ebp, esp, are known as the Function Prologue. They are the standard architectural glue that allows C and Assembly to talk to each other without destroying the system.
;inside the CPU, ESP (Extended Stack Pointer) is the live, active laser pointer. It always points to the absolute top plate on the stack. Whenever we push, ESP automatically moves up. Whenever we pop, ESP automatically moves down.
;because ESP is constantly bouncing around as our code runs, it is a terrible reference point. If we want to grab the first parameter passed from C, we might think it is at [esp+4]. But if we push a variable a microsecond later, that parameter is now suddenly at [esp+8]. It is like trying to measure a room with a tape measure that keeps changing its length.
;EBP (Extended Base Pointer) is the solution. It acts as an immovable object
;when our C code calls jump_to_usermode, the C function that called it was already using EBP as its own immovable object to keep track of its own variables.
;if we just overwrite EBP, we will permanently blind the parent C function. So, the absolute first thing we do is push ebp. We physically save the parent's immovable object onto the stack so we can give it back later.
;now that the parent's immovable object is safely locked away on the stack, we are free to use the EBP register.
;we look at exactly where ESP is right now, and we copy that address into EBP.
;EBP is now frozen. It acts as a perfectly still, immovable bookmark for the exact moment our function began.
;because EBP never moves, we can use it to mathematically map out the stack perfectly, no matter how many times ESP bounces around later.
;because of the C Calling Convention, the stack immediately after mov ebp, esp always looks exactly like this:
;[ebp]-> the parent's saved immovable object (which we just pushed).
;[ebp + 4]   -> the Return Address (where to jump back to when the function finishes, pushed automatically by the CPU's call instruction).
;[ebp + 8]   -> parameter 1 (entry_point).
;[ebp + 12]  -> parameter 2 (user_esp).
;by using ebp as our immovable object, we can confidently write mov ebx, [ebp+8] to grab our first C parameter, knowing it will always be mathematically sound.

;grab parameters passed form c
mov ebx, [ebp+8]  ; the entry_point (EIP)
mov ecx, [ebp+12] ; the user_esp (Stack Pointer)
; We need to update the data segment registers before the drop
mov ax, 0x23; 0x23 is our User Data Selector
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

; push the 5-plate iret stack!
push 0x23; 1. Push User Data Segment
push ecx; 2. Push User Stack Pointer
push 0x202; 3. Push EFLAGS (Interrupts On)
push 0x1B; 4. Push User Code Segment
push ebx; 5. Push Instruction Pointer (EIP)
;drop
iret ;CPU yanks the 5 plates and falls into Ring 3

mov esp, ebp ;reset as it was
pop ebp ; return back the immovable object back to the parent
ret; back to c

;tell the CPU hardware exactly which GDT entry holds the TSS this is done by ltr
global tss_flush
tss_flush:
mov ax, 0x2B;index 5 in the GDT (5 * 8 = 40 = 0x28) | RPL 3 = 0x2B
ltr ax;Load Task Register
;violently jams 0x2B into the CPU's Task Register. The CPU now permanently knows exactly where to look when an interrupt fires in Ring 3.
ret

extern isr_handler ;this is the master C function we will write next
isr_common_stub:
pusha; pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
mov ax, ds; lower 16-bits of eax = ds.
push eax; save the data segment descriptor
mov ax, 0x10; load the Kernel Data Segment descriptor
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

push esp; pass the entire stack frame as a parameter to C!
call isr_handler
add esp, 4; clean up the parameter we pushed

pop ebx; reload the original data segment descriptor
mov ds, bx
mov es, bx
mov fs, bx
mov gs, bx

popa; pop edi,esi,ebp...
add esp, 8; cleans up the pushed error code and pushed ISR number
sti
iret; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP


global isr128
isr128:
cli; disable interrupts so we aren't interrupted while setting up the stack
push 0; push a dummy error code (to keep the stack structure consistent with exceptions)
push 128; push the Interrupt Number (0x80)
jmp isr_common_stub ; jump to the massive save/restore logic above!









