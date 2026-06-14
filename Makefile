
CC = gcc
# -m32: Force 32-bit compilation.
# -march=i386: Use base x86 instruction set for maximum hardware compatibility.
# -ffreestanding: Tell the compiler there is no host OS and no standard library (<stdio.h>).
# -fno-pie / -fno-pic: Disable position-independent code (security features that break bare-metal).
CFLAGS = -m32 -march=i386 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-builtin

ASM = nasm
#Compile assembly to standard 32-bit ELF object files so the Linker can read them.
ASMFLAGS = -f elf32

LD = ld
# -m elf_i386: Force the Linker to output a 32-bit ELF binary.
# -T linker.ld: Feed the Linker our custom blueprint.
LDFLAGS = -m elf_i386 -T linker.ld

#Typing 'make' will sequentially clean old files, build the ISO, and launch QEMU.
all: clean build/ng-os.iso run


#Compile boot assembly
build/boot.o: src/boot/boot.asm
	mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

#Compile interrupt assembly
build/interrupt.o: src/interrupt.asm
	mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

#Compile C files
build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

#Stitch all the .o files together into one final kernel.bin file.
#The order here doesn't strictly matter because linker.ld enforces the layout.
build/kernel.bin: build/boot.o build/interrupt.o build/kernel.o build/idt.o build/keyboard.o build/timer.o build/vga.o build/gdt.o build/memory.o build/pmm.o build/paging.o build/heap.o build/pit.o build/task.o build/isr.o
	$(LD) $(LDFLAGS) $^ -o $@


build/ng-os.iso: build/kernel.bin
	# 1. Create a temporary folder structure that mimics a GRUB boot CD
	mkdir -p iso/boot/grub
	# 2. Copy our GRUB configuration text file from the root into the CD layout
	cp grub.cfg iso/boot/grub/grub.cfg
	# 3. Copy our compiled kernel into the CD layout
	cp build/kernel.bin iso/boot/kernel.bin
	# 4. Use the xorriso tool to wrap the folder into a bootable .iso file
	grub-mkrescue -o build/ng-os.iso iso

run: build/ng-os.iso
	# Launch QEMU with a virtual CD-ROM drive instead of a raw hard drive
	qemu-system-i386 -cdrom build/ng-os.iso -m 2G

clean:
	rm -rf build/*
	rm -rf iso/boot/kernel.bin