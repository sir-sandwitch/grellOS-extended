nasm -f elf64 multiboot_header.asm
nasm -f elf64 kernel.asm
objcopy -O elf64-x86-64 kernel.o kernel.elf