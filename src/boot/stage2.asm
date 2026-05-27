section .text ;This tells the linker where the executable code is.
extern kernel_main ;extern stands for External, logic: kernel_main is present in kernel.c so leave memory for it and execute.
bits 16 ;we are still in 16 bit mode
;testing update: passing stage2.asm to the linker needs to be compiled with the kernel as a structured object file (ELF32), not raw binary.
;So we have to remove the line org 0x9000 because ELF files don't use org. The linker script handles the memory address now.
;but i'll be commenting all the old instructions so that it makes eaiser to learn & refer.

;org 0x9000 ;where the code exactly will be present based on previous file/sector

xor ax, ax      ; Set AX to 0 quickly, xor ax, ax is the fastest, most efficient way to set a register to zero.
mov ds, ax      ; Set Data Segment to 0
mov es, ax      ; Set Extra Segment to 0
; Clears segment registers to remove random garbage values left behind by the BIOS.
; 'xor ax, ax' mathematically sets AX to 0 faster and with fewer bytes than 'mov ax, 0'.
; We copy this 0 into DS and ES because 16-bit memory math multiplies segments by 16.
; Forcing segments to 0 ensures the CPU looks exactly at our 'org 0x7C00' memory offset.
; This prevents the hardware from reading the wrong RAM addresses and crashing the system.


mov ah, 0x0e
mov si, stage2_msg
print_loop:
lodsb
cmp al,0
je unlock_a20 ; jump if equal, jump to unlock a20
int 0x10
jmp print_loop

unlock_a20:
; cannot use mov to write to hardware ports, must use the in and out instructions.
;The motherboard listens on port 0x92. motherboard's physical System Control Port A, exactly 1 byte (8 bits) wide.
;Bit 0 Fast Reset: If you write a 1 to this bit, it sends an electrical pulse straight to the CPU's reset wire, instantly rebooting your entire computer.
;Bit 1 A20 Gate Switch: 0 means the 21st memory wire is locked shut (trapping the computer in 1MB of RAM).
;1 means the wire is open, unlocking all your laptop's RAM so you can enter 32-bit and 64-bit modes safely.
in al, 0x92 ;reads data from a port into AL. The second bit (Bit 1) of the byte stored at this port controls the A20 line.
;If it's a 0, A20 is disabled. If it's a 1, A20 is enabled.
;MODIFY bit 1 (set it to 1) using a logical OR.
or al,0x02 ; 0x02 in binary is 00000010, which targets the A20 switch.
out 0x92,al ;writes data from AL to a port.

cli    ;Disable all hardware interrupts! (without this faced a irritating problem...)
; The motherboard has a Programmable Interval Timer (PIT) that fires a hardware
; interrupt 18.2 times every second. In 16-bit Real Mode, the BIOS catches this
; interrupt, updates the system clock, and returns control to our code.
;
; The moment we flip the PE bit in cr0 to enter 32-bit Protected Mode, we lose
; the BIOS entirely. If the timer ticks right as we are transitioning, the CPU
; will pause our code, look for a 32-bit Interrupt Descriptor Table (IDT) to
; handle the timer, realize the IDT doesn't exist yet, and instantly Panic/Triple-Fault.
; (This is why our print loop died mid-sentence at "Stage 2 l...").
;
; FIX: 'cli' (Clear Interrupt Flag)
; This instruction modifies the CPU's EFLAGS register, explicitly telling the
; processor to go deaf. It blocks all maskable hardware interrupts (like the
; timer and keyboard). We execute this right before loading the GDT to create
; a perfectly silent, safe vacuum for the CPU to transition into 32-bit mode.
; We will not use 'sti' (Set Interrupt Flag) to turn them back on until we
; have written our C kernel's IDT to safely handle them.

lgdt [gdt_descriptor] ;Load Global Descriptor Table, it hands the table directly to the CPU's internal hardware.
;in NASM, if we write lgdt gdt_descriptor, the assembler thinks we want to load the literal memory address of the descriptor into the CPU register.
;But the CPU actually wants to read the data at that address. To tell NASM to dereference the pointer (like *ptr in C), we must wrap the label in square brackets.
;The CPU cannot just guess where we wrote the GDT in our assembly file. we have to pass lgdt a specific 6-byte pointer structure

;the GDT is loaded, but the CPU is ignoring it. We have to manually flip the switch to turn it into a 32 bit processor.
;inside the processor is Control Register 0 (cr0), the absolute first bit of this register is the Protection Enable (PE) bit.
;we directly cannot modify cr0, so we move it into a general purpose reg. Using eax, the 32 bit version of ax, because the CPU actually supports 32-bit registers in 16-bit mode if we force it
mov eax,cr0 ; cr0 to eax
or eax,0x01 ; logical OR instruction to flip the first bit to 1
mov cr0,eax ; back to cr0
;the CPU is in 32-bit Protected Mode.

;as now the CPU is in 32 bit mode, we need to do a far jump... why?
;the CPU pre-fetches instructions to run faster. Its internal pipeline is currently full of 16-bit instructions it decoded a microsecond ago. if it executes them now, the CPU will crash.
;we must forcefully flush the pipeline. We do this by executing a Far Jump. A standard jmp just moves the instruction pointer. A Far Jump updates both the Code Segment (CS) register AND the instruction pointer.
;because we are in Protected Mode now, the CS register no longer holds a mathematical segment. It holds an Offset pointing into our GDT.
;entry 1 is the Null Descriptor (Offset 0x0)(0-7). [The GDT rule]
;entry 2 is the Code Segment (Offset 0x08 - because the Null descriptor is 8 bytes long)(8-15).
;entry 3 is the Data Segment (Offset 0x10)(16-23)(0x10 in hex).

jmp dword 0x08:init_pm ;To flush the CPU pipeline and enter 32-bit mode, we execute a Far Jump: 'jmp 0x08:init_pm'.
; Because this instruction is written inside our 'bits 16' zone, the Assembler
; normally compiles it assuming 'init_pm' is a small 16-bit memory address.
; However, because we are passing this file to an ELF32 Linker to connect it
; with our C Kernel, the Linker calculates 'init_pm' as a massive 32-bit address.
;
; Without the 'dword' keyword, the assembler truncates (chops off) the top half
; of the 32-bit address. The CPU jumps to the wrong location in memory, hits
; empty garbage data, and crashes silently.

;Adding 'dword' (Double Word / 32 bits) forces the 16-bit assembler to allocate
; enough space for the full 32-bit memory address provided by the ELF Linker.
; This guarantees we land perfectly at the 'init_pm' label inside our C-linked RAM.

;we must jump to the Code Segment (0x08) and absolutely cannot jump to the Data Segment (0x10) because of hardware-enforced security.
;the code segment access byte says 10011010b (4th bit from the right is 1, meaning Executable).
;data segment access byte says 10010010b (4th bit from the right is 0, meaning Not Executable).
;when we execute a Far Jump, we are forcing a new value into the CS (Code Segment) register. The CPUs Instruction Pointer uses the CS register to fetch the next command.
;If we try to jump to 0x10 (Data Segment), the CPU sees that the Executable bit is 0. It instantly throws a hardware exception (a General Protection Fault). we haven't written an error handler yet, the CPU will simply panic, triple-fault, and reboot the machine. (this line will be edited once handler is done.)

;imagine running a web server, server receives data from the internet and stores it in variables inside our RAM. That RAM is mapped to our Data Segment. If a hacker sends a string that actually contains malicious compiled Assembly code, and the CPU was allowed to execute the Data Segment, the hacker would instantly take control of your computer.
;by strictly separating Code (Read/Execute) from Data (Read/Write), the hardware ensures that even if a hacker injects malicious code into our variables, the CPU will physically refuse to execute it.

;0x08 is the GDT Offset. It is not a memory address. It is an index. It literally means "Look at the 8th byte inside the GDT table."
;Our Far Jump (jmp 0x08:init_pm) tells the CPU: "Go look at the rules in GDT offset 0x08, apply those rules, and then jump to the physical memory address of the init_pm label."
bits 32; from here the 32 bits starts
init_pm:
mov ax,0x10;Data Segment offset
;in 16-bit mode, these segment registers held raw memory locations. In 32-bit mode, they hold an offset (index pointer) into our GDT.
;CPU uses different segment registers for different jobs: ds for regular data variables, ss for the stack, and es/fs/gs for extra data operations.
;we load 0x10 into every single one of them to tell the hardware: "For every type of memory operation from now on, use the Data Descriptor security rules we set up in the GDT."
mov ds, ax ;Update all data segment registers to the GDT Data Segment
mov ss, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ebp, 0x90000 ;setting up a massive new 32-bit stack
;ebp stands for Extended Base Pointer. It is a 32-bit register used to point to the very bottom of our current function's stack frame.
;old 16-bit stack at 0x7C00 was tiny and dangerous. Now that we have access to gigabytes of RAM, we want to give our OS a massive, completely empty workspace.
;on standard PCs, the memory region up to 0x9FFFF is completely free. Setting our stack base to 0x90000 guarantees that our stack has nearly 64 Kilobytes of clear RAM to grow downwards into without hitting your code.
mov esp, ebp ;esp stands for Extended Stack Pointer
;when we start a brand-new stack, it must be completely empty. By making esp match ebp (0x90000), we are initializing the stack. The very next time we run a push instruction, esp will safely subtract memory from 0x90000 and store the data.

;Code-Update until the next ----> , (note: this is an old code used for testing, its commented out and not deleted to make it suitable to learning and decoding.)
;as we are officially into 32 bit mode,
;In 16-bit mode, we could ask the BIOS to print letters for us using int 0x10.
;the moment we flipped cr0 to 1, we entered 32-bit Protected Mode. We instantly lost access to the BIOS. If we try to use int 0x10 to print a message to say "Hey!", the CPU will crash.
;we have to bypass the BIOS entirely and write data directly into the computer's physical VGA Video Memory.
;the motherboard physically maps the text display to the exact RAM address 0xB8000. It is a grid of memory. Every character on the screen takes up exactly two bytes in this grid.
;The first byte is the ASCII character (e.g., 'X'). The second byte is the color code (e.g., 0x0f for white text on a black background).
;To print the letter 'X' in the top-left corner of the screen, we don't call a function. We literally write two bytes directly to 0xB8000 and 0xB8001.
;mov ebx,0xB8000 ; ebx cause we are in 32 bit mode... obv, storing the address of the physical address 0xB8000 points straight to the memory chips inside your video card
;mov al,'D';the letter to print
;mov ah, 0x0f ; the color code( 0=black, f=white).
;mov [ebx],ax ; deliver the ax(al,ah) at the address of ebp ie VGA memory address to display on screen.
;---->

call kernel_main ;go to C code and run the kernel
;We add a backup safety net here just in case the C code fails to save from crash
jmp $ ;Jump to the exact line we are currently standing on. (infinite loop)



stage2_msg: db 'Stage 2 loaded lets go... excited btw :)',0
;We do NOT need the 512-byte padding or the 0xaa55 signature in this file. The BIOS only checks Sector 0 for that. Stage 2 can be any size

;GDT
gdt_start: ;marks the beginning of the table
gdt_null: ;the mandatory empty descriptor
dd 0x0
dd 0x0
gdt_code: ;the code segment rules
dw 0xffff ;the first 16 bits of the Limit.
dw 0x0 ;the first 16 bits of the Base Address.
db 0x0 ;the next 8 bits of the Base Address.
db 10011010b ;Access Byte. This is binary for: Present (1), Ring 0 (00), Executable Code (1). Turning these specific switches on tells the hardware: "This block contains executable code, and only the Kernel (Ring 0) is allowed to touch it."
db 11001111b  ;The Flags & Limit. This sets 32-bit mode (1), 4KB page granularity (1), and the final 4 bits of the Limit (1111). More switches. This forces the CPU into 32-bit mode instead of 16-bit mode, and holds the final leftover bits of our Limit.
db 0x0 ; The final 8 bits of the Base Address.
;When the CPU reads these lines, it automatically glues the scattered pieces back together to recreate the full 0 to 4GB boundaries.

gdt_data: ;The data segment rules
;The Data Segment is identical to the Code Segment, because it also spans from 0 to 4GB.
;The only difference is one single switch in the middle of that binary block:
dw 0xffff ;the first 16 bits of the Limit.
dw 0x0 ;the first 16 bits of the Base Address.
db 0x0 ;the next 8 bits of the Base Address.
;code uses: 10011010b (The 1 in the middle means Executable)
;data uses: 10010010b (That bit is flipped to 0, meaning Storage Only / Do Not Execute)
;note: this simple tweak stops a hacker from trying to execute malware hidden inside our variables.
db 10010010b ;Access Byte. This is binary for: Present (1), Ring 0 (00), Executable Code (1).
db 11001111b  ;The Flags & Limit. This sets 32-bit mode (1), 4KB page granularity (1), and the final 4 bits of the Limit (1111).
db 0x0 ; The final 8 bits of the Base Address.
gdt_end: ;Marks the end of the table so we can calculate its total size

;need a tiny 6-byte structure that tells the CPU where to find the table.
gdt_descriptor: ;hold the address of our table.
dw gdt_end - gdt_start - 1 ;word, containing the total size of the GDT minus 1. To tell the CPU exactly how many bytes to read.
dd gdt_start ;double-word, containing the memory address of the table.
;gdt_start so the CPU knows the exact coordinate where the table begins in RAM.

;try to connect the dots, lgdt is the thing that loads the gdt in the cpu, ie the pointer means the address in the cpu from that it calculates the size and start.
