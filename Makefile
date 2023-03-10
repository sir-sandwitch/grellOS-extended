default: run

.PHONY: default build run clean

build/multiboot_header.o: multiboot_header.asm
	mkdir -p build
	nasm -f elf64 multiboot_header.asm -o build/multiboot_header.o

build/boot.o: boot.asm
	mkdir -p build
	nasm -f elf64 boot.asm -o build/boot.o

build/kernel.o: kernel.c
	mkdir -p build
	gcc -fno-stack-protector -m64 -c kernel.c -o build/kernel.o

build/kernel.bin: build/multiboot_header.o build/boot.o linker.ld build/kernel.o
	ld -n -o build/kernel.bin -T linker.ld build/multiboot_header.o build/boot.o build/kernel.o

build/os.iso: build/kernel.bin grub.cfg
	mkdir -p isofiles/boot/grub
	cp grub.cfg isofiles/boot/grub
	cp build/kernel.bin isofiles/boot/
	grub-mkrescue -o build/os.iso isofiles

build: build/os.iso

run: build/os.iso
	qemu-system-x86_64 -cdrom build/os.iso

clean:
	rm -rf build
