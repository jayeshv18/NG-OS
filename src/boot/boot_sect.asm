

; [ARCHIVED] Legacy 16-bit bootloader loop kept strictly for revision and reference.
; Production builds utilize GRUB for memory management and kernel loading.
; Excellent educational blueprint for understanding the low-level x86 handoff.

; NOTE: This code is currently inactive. It is retained solely as a reference




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
;The BIOS int 0x13 interrupt uses the CPU stack internally. If we don't explicitly define where the stack is, the BIOS might use random memory and overwrite our bootloader, causing a crash.
org 0x7c00 ;The BIOS always loads us at exactly 0x7C00, we create a variable, NASM needs to know where in RAM this code will physically live so it can calculate the pointer.

;reading the disk requires more coordination than printing text, gotta setup a stack so that int 0x13 doesnt overwrite our code.
;we have to load six different registers with coordinates, execute the interrupt, and jump to the newly loaded code.
;this about it, you cant fit all in one sector, so we broke the bootloader into parts and now these parts are to be connected using jmp while executing so that the bootloader is complete.
mov bp,0x7c00 ;Base point of the stack
mov sp,bp ; initially it'll hold the base pointer valuse and then it'll grow

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
je load_disk ;if yes, we are done printing. Jump to the load_disk
int 0x10 ;int 0x10, trigger. This executes the BIOS code.
jmp print_loop;jump back to the top of the loop to get the next letter!

;BIOS disk reading
;Memory and Loops are 0-indexed (they start counting at 0).Disk Sectors are 1-indexed (they start counting at 1).
;sector 2 is the very next to sector 1 where stage 1 is stored.

;bootloader is the absolute first thing written onto the drive, it sits at the very beginning of the hardware: Cylinder 0, Head 0, Sector 1 (Stage 1),
;followed immediately by Cylinder 0, Head 0, Sector 2 (Stage 2).

load_disk:
mov ah, 0x02 ;reading the next sector ie the next stage of the boot
mov al, 30 ;number 30 tells the BIOS exactly how many sectors (blocks of 512 bytes) to grab from the disk in one single scoop. 30 gives plenty of space(15kb)
;15 because your Stage 2 code is too big to fit in one 512-byte sector, but it easily fits within 7.5 KB. You are telling the BIOS: "Go to Sector 2, grab the next 15 sectors in a row, and load them all into memory for me."
;if youre bored or not able to understand then think about me :(
mov ch, 0 ;cyclinder 0. Hard-drives follow CHS mapping(cyclinder,head,sector).
mov dh, 0 ;head 0
mov cl, 2 ; start reading from second sector because the first is already done... i hope you get it, try to connect the dots.
mov bx, 0x9000 ;RAM dest for stage 2
int 0x13 ;execute the read disk.

main: ;We need to create a label (main: ) and instruct CPU to jump back to that position forever so that it doesnt fall of the 512 and execute garbage.
jmp 0x9000      ; Jump to the code we just loaded.
;there is important thing to realize, the BIOS will copy the first sector to 0x7c00 and execute then later we have the whole RAM free,
;we cant tightly pack it next to each other because the CPU mmu is physically 4kb, as grid.

;now the bios doesnt know where to cpy the next instruction, so we instruct it.
;setting mov bx, 0x9000, we are handing the BIOS a container and saying: "Go to the disk, grab those 15 sectors, and copy them exactly into RAM address 0x9000.
;When we write our separate Stage 2 assembly file, "org 0x9000" will tell the assembler Stage 2 will live at 0x9000

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

