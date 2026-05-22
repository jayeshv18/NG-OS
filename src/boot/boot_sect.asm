;memory is divided into physical chunks called "sectors." A standard sector holds exactly 512 bytes of data.
;Sector 0 is the absolute first chunk of data on the entire disk.
;the motherboard's BIOS doesn't know how to read files. It blindly reaches out to the USB drive, grabs only Sector 0,
;copies those 512 bytes into your computer's RAM at address 0x7C00, and tells the CPU to start executing it.
;so as the BIOS pnly loads the sector 0, boot_sect.asm has a strict absolute limit of 512 bytes,
;the only job of boot_sect.asm is to act as a loader for the next stage. It sets up a basic memory stack,
;prints a quick "Loading..." message to the screen, and then reads Sector 1 from the USB drive into RAM.

;The motherboard has tiny pieces of code burned into its silicon ROM chip.
;An "interrupt" is an assembly instruction (like int 0x13) that pauses your code and triggers the motherboard's built-in code.
;int 0x10 tells the motherboard to draw a character on the screen.
;int 0x13 tells the motherboard to read the disk.
;Once boot_sect.asm uses int 0x13 to load stage2.asm into memory, it executes a jmp (jump) instruction to pass control over to it... stage2.asm

bits 16 ;When NASM translates text into binary, t needs to know what era of CPU it's targeting. Since the BIOS wakes the computer up in Real Mode (16 bit)
; org stands for where exactly our code will be loaded.
org 0x7c00 ;The BIOS always loads us at exactly 0x7C00, we create a variable, NASM needs to know where in RAM this code will physically live so it can calculate the pointer.

;if we try to print something after main the cpu will get stuck in an infite loop and never reach our instruction..so, must put before.
;we dont have any print or any functions here, so to print a character in 16bit realmode, we have to load specific values into the CPU registers, to tell the BIOS what to do.
; BIOS contains pre-written basic utility programs stored directly on its chip. "0x10" is slot for video graphics and text display.
;to use this we need to use interrupt (int 0x10).

;the CPU has a 16 bit Accumulator register is called ax. It is split into two 8-bit halves: ah (high byte) and al (low byte).
;ah must contain 0x0e, this code tells BIOS be prepared we gonna print and move the cursor forward.
;al must contain the character, the letter we going to print will be here in this.

mov ah,0x0e
;SI register is specifically designed for pointing to strings in memory, we will load the address of our string into SI.
mov si, boot_msg;must define the string in the raw memory of the file using a label (like boot_msg:), and then tell si to point to that label's memory address. load the address of your string into the SI register
print_loop:
;lodsb (Load String Byte) is crazy when executed, the CPU It reads the byte at the memory address stored in SI and puts it into the AL register (which is exactly where int 0x10 wants it).
;also it automatically increments SI by 1, so it's pointing at the next character.
lodsb
cmp al,0 ;did we just load the null terminator?
je main ;if yes, we are done printing. Jump to the infinite loop aka main.
int 0x10 ;int 0x10, trigger. This executes the BIOS code.
jmp print_loop;jump back to the top of the loop to get the next letter!

main: ;We need to create a label (main: ) and instruct CPU to jump back to that position forever so that it doesnt fall of the 512 and execute garbage.
jmp main ;The CPU will jump exactly to wherever 'main' is located

;DATA SECTION
;we define our string here in memory, ending with a 0.
boot_msg: db 'Loading NG-OS...',0

;PADDING & SIGNATURE
;As the BIOS needs sector 0 exactly as 512,
times 510-($-$$) db 0 ;repeat the data byte '0' for 510 minus the size of the code we've written so far.
;dw stands for define word, command used to reserve and initialize 16 bits (2 bytes) of data directly into your compiled file.
;0xAA55 is made of two separate hex bytes (0xAA and 0x55), dw to combine them perfectly into a single, 2-byte instruction.
dw 0xAA55 ;inserts the mandatory 0xAA55 boot signature so the motherboard BIOS recognizes this as a valid bootloader,
;BIOS, It explicitly demands that byte 511 and byte 512 contain the exact values 0x55 and 0xAA (written as 0xAA55 in memory due to little-endian formatting).

