default: run

.PHONY: default build run clean cargo

target/multiboot_header.o: src/asm/multiboot_header.asm
	mkdir -p target
	nasm -f elf64 src/asm/multiboot_header.asm -o target/multiboot_header.o

target/boot.o: src/asm/boot.asm
	mkdir -p target
	nasm -f elf64 src/asm/boot.asm -o target/boot.o

target/kernel.o: src/kernel.c
	mkdir -p target
	gcc -c src/kernel.c -o target/kernel.o -std=gnu17 -ffreestanding -Wall -Wextra -Iinclude

target/kernel.bin: target/multiboot_header.o target/boot.o src/asm/linker.ld target/kernel.o
	ld -n -o target/kernel.bin -T src/asm/linker.ld target/multiboot_header.o target/boot.o target/kernel.o

target/os.iso: target/kernel.bin src/asm/grub.cfg
	mkdir -p target/isofiles/boot/grub
	cp src/asm/grub.cfg target/isofiles/boot/grub
	cp target/kernel.bin target/isofiles/boot/
	grub-mkrescue -o target/os.iso target/isofiles

cargo:
	xargo build --release --target=x86_64-unknown-grellos-gnu

build: target/os.iso

run: target/os.iso
	qemu-system-x86_64 -cdrom target/os.iso

clean:
	rm -rf target
