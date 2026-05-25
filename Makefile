#A Makefile works by defining "targets" (the files you want to create), their "dependencies" (the files needed to make them), and the terminal command required to build them.
#A Makefile is a simple text file that acts as an automation script for building software.
#Makefile is that it acts as an intelligent blueprint. Instead of typing a massive 50-word command every time you change a single line of C code, you just type make in your terminal.

#Instead of writing x86_64-elf-gcc and all those flags over and over, we store them in variables at the top of the file.

CC = gcc
CFLAGS = -m32 -march=i386 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-builtin

# Define assembler and flags
ASM = nasm
ASMFLAGS_BIN = -f bin
ASMFLAGS_ELF = -f elf32

#you reference a variable by wrapping it in $().
#The syntax is always Target : Dependency.
#Target=build/kernel.o The file we want to create.
#Dependency=src/kernel.c The file required to make it.
# Define linker and flags
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

#Targets
all: clean build/ng-os.img run

#Compile Sector 1
build/boot_sect.bin: src/boot/boot_sect.asm
	mkdir -p build
	$(ASM) $(ASMFLAGS_BIN) $< -o $@

#Compile Stage 2
build/stage2.o: src/boot/stage2.asm
	mkdir -p build
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

#Compile C Kernel
build/kernel.o: src/kernel.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

#Compile VGA Driver
build/vga.o: src/vga.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

#Link it all into a flat binary
build/kernel.bin: build/stage2.o build/vga.o build/kernel.o
	$(LD) $(LDFLAGS) $^ -o $@

# the final OS image
build/ng-os.img: build/boot_sect.bin build/kernel.bin
	cat $^ > $@

# Run QEMU
run: build/ng-os.img
	qemu-system-x86_64 -drive format=raw,file=$<

# Clean build directory
clean:
	rm -f build/*