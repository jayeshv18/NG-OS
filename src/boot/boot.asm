; this is a GRUB based boot file... we want GRUB to load our operating system and hand us the Memory Map.

; MULTIBOOT CONSTANTS
MULTIBOOT_NUM equ 0x1BADB002 ;Magic number GRUB explicitly scans for, This is a hardcoded hex value defined by the Free Software Foundation. GRUB sees this, it stops and says, "Wait, this might be a kernel."
MULTIBOOT_FLAGS equ 0x03 ;
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_NUM + MULTIBOOT_FLAGS) ;Must equal 0 when added together
;0x1BADB002 could just randomly appear in a normal text file or image. To prove it is intentional, the Multiboot standard demands a mathematical checksum.


section .multiboot
bits 32
align 4                     ; CRITICAL CONSTRAINT: Intel hardware and GRUB require 4-byte alignment
dd MULTIBOOT_NUM          ; Write the 32-bit magic token
dd MULTIBOOT_FLAGS          ; Write the 32-bit configuration flags
dd MULTIBOOT_CHECKSUM       ; Write the 32-bit validation checksum
;if these three numbers are present and the math checks out, GRUB takes total control.
;it loads our kernel into RAM, flips the CPU to 32-bit Protected Mode, puts the Memory Map pointer into ebx, puts a confirmation magic number into eax, and jumps to our code.


;GRUB does not give us a safe stack memory space. We have to carve it out yourself.
section .bss ;uninitialized data
align 16
stack_bottom:
;Reserve Byte aka resb
resb 16384 ;carving out 16 Kilobytes (16384 bytes) of empty space for the stack using the resb command.
stack_top: ;the label MUST go here so the stack can grow downwards safely!


section .text
extern kernel_main
global _start ;universal entry point

_start:
mov esp, stack_top
push ebx ;the ebx register onto the stack immediately. This contains the physical address of GRUB's Memory Map. If we dont push it, the C compiler will overwrite it, and wwe will lose the map forever.
push eax ;the eax register (contains GRUB's confirmation magic number).
cli ;Disable hardware interrupts with cli (clear interrupt flag)
call kernel_main

;if the kernel ever accidentally returns, put the CPU into a permanent freeze
.hang:
cli
hlt
jmp .hang



