/* Tell the linker that kernel_main is our entry point */
/*
Linker's Instruction Manual on exactly how to pack those boxes into a single file (kernel.elf) so that when it is loaded into the computer's RAM,
everything sits exactly where the CPU expects it.
*/

ENTRY(kernel_main)

SECTIONS
{
    /* Tell the linker this entire binary will be loaded at 0x9000 in RAM */
        . = 0x9000;
        /*starting coordinate in memory. It tells the linker: "Calculate all memory addresses assuming this entire file will be unpacked and dropped into the physical RAM chips exactly at address 0x9000.*/

   .text : ALIGN(4K) /*make sure it starts perfectly at a clean 4KB hardware page boundary so the CPU can read it fast and secure it.*/
       {
           /* Force the stage2 assembly code to be the absolute first thing in the binary */
           build/stage2.o(.text) /*strict packing rule, logic: Take the Stage 2 assembly code and pack it at the absolute front of the file... if this not done, then it'll accidently boot c code first and crash*/

           *(.text)/* Put the C kernel code right after it */
       }

       /* Standard C data sections */
       .rodata : ALIGN(4K) { *(.rodata) } /*holds our permanent text strings, It is locked so it can never be changed while the OS runs.*/
       .data : ALIGN(4K) { *(.data) } /*holds our regular variables that have a starting value*/
       .bss : ALIGN(4K) { /*holds variables that start at zero. Instead of wasting space storing thousands of actual zeros on our disk, Just reserve so and so empty space in RAM when we boot up."*/
           *(COMMON)
           *(.bss)
       }
}