#!/bin/bash
# buildimg.sh - Build raw disk image for x86-64 ExoKernel

set -e

# Directories
BUILD_DIR="build"
SRC_DIR="src"
DRIVERS_DIR="drivers"
OUTPUT_IMG="exokernel.img"
OUTPUT_BIN="$BUILD_DIR/kernel.bin"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ExoKernel x86-64 Disk Image Build ===${NC}"

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo -e "${BLUE}[1/4] Assembling bootloader...${NC}"
nasm -f bin -o boot.bin ../$SRC_DIR/boot.asm

echo -e "${BLUE}[2/4] Assembling kernel entry...${NC}"
nasm -f elf64 -o kernel_entry.o ../$SRC_DIR/kernel_entry.asm

echo -e "${BLUE}[3/4] Compiling C kernel and drivers...${NC}"
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/kernel.c -o kernel.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/shell.c -o shell.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/fastfetch.c -o fastfetch.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/libc.c -o libc.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/pci.c -o pci.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$SRC_DIR/net.c -o net.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$DRIVERS_DIR/vga.c -o vga.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$DRIVERS_DIR/ata.c -o ata.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$DRIVERS_DIR/e1000.c -o e1000.o
gcc -target x86_64-pc-none-elf -ffreestanding -fno-stack-protector -fno-builtin \
    -Wall -Wextra -m64 -c ../$DRIVERS_DIR/fs.c -o fs.o

echo -e "${BLUE}[4/4] Linking kernel binary...${NC}"
ld -T ../linker_grub.ld -m elf_x86_64 -o kernel.bin kernel_entry.o kernel.o \
    shell.o fastfetch.o libc.o pci.o net.o vga.o ata.o e1000.o fs.o \
    --oformat binary

# Create raw disk image
echo -e "${BLUE}Creating raw disk image...${NC}"
dd if=/dev/zero of=../$OUTPUT_IMG bs=512 count=2880 2>/dev/null
dd if=boot.bin of=../$OUTPUT_IMG bs=512 count=1 conv=notrunc 2>/dev/null
dd if=kernel.bin of=../$OUTPUT_IMG bs=512 seek=2 conv=notrunc 2>/dev/null

echo -e "${GREEN}✓ Build complete!${NC}"
echo -e "${GREEN}Disk image: ../$OUTPUT_IMG${NC}"
echo -e "${GREEN}Kernel binary: $OUTPUT_BIN${NC}"
