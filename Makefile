#A Makefile works by defining "targets" (the files you want to create), their "dependencies" (the files needed to make them), and the terminal command required to build them.
#A Makefile is a simple text file that acts as an automation script for building software.
#Makefile is that it acts as an intelligent blueprint. Instead of typing a massive 50-word command every time you change a single line of C code, you just type make in your terminal.

#Instead of writing x86_64-elf-gcc and all those flags over and over, we store them in variables at the top of the file.
all: ng-os.elf #It will look at all, realize it needs ng-os.elf, and run the linker script to give you your final kernel binary!
CC = gcc
CFLAGS = -ffreestanding -nostdlib -mno-red-zone -fno-pic -fno-pie -mcmodel=kernel

#you reference a variable by wrapping it in $().
#The syntax is always Target : Dependency.
#Target=build/kernel.o The file we want to create.
#Dependency=src/kernel.c The file required to make it.
build/kernel.o: src/kernel.c
		mkdir -p build
		$(CC) $(CFLAGS) -c src/kernel.c -o build/kernel.o #compiler, pass our safety flags, tell it to compile without linking (-c), and specify the output file (-o).

build/serial.o: src/serial.c
		mkdir -p build
		$(CC) $(CFLAGS) -c src/serial.c -o build/serial.o

#Once we have our compiled object file (kernel.o), we need to run it through the linker script (linker.ld) to organize the memory into our final, bootable OS executable (ng-os.elf).
ng-os.elf: build/kernel.o build/serial.o
		$(CC) -T linker.ld -o ng-os.elf $(CFLAGS) -no-pie build/kernel.o build/serial.o
	#-T linker.ld: This is the magic flag. It tells the GCC linker, "Do not use your default Linux memory map. Use the custom 4KB-aligned blueprint we wrote in linker.ld."
clean:
	rm -f build/*.o ng-os.elf