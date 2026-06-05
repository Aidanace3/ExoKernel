#!/bin/bash
# buildiso.sh - Build ISO image for x86-64 ExoKernel

set -e

# Directories
BUILD_DIR="build"
SRC_DIR="src"
DRIVERS_DIR="drivers"
OUTPUT_ISO="exokernel.iso"
OUTPUT_BIN="$BUILD_DIR/kernel.bin"
ISO_DIR="$BUILD_DIR/iso_root"
GRUB_DIR="$ISO_DIR/boot/grub"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ExoKernel x86-64 ISO Build ===${NC}"

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
    shell.o fastfetch.o libc.o pci.o net.o vga.o ata.o e1000.o fs.o

# Setup ISO directory structure
echo -e "${BLUE}Creating ISO directory structure...${NC}"
rm -rf $ISO_DIR
mkdir -p $GRUB_DIR

# Copy kernel to ISO
cp kernel.bin $ISO_DIR/boot/

# Create GRUB configuration
cat > $GRUB_DIR/grub.cfg << 'EOF'
menuentry 'ExoKernel x86-64' {
    multiboot /boot/kernel.bin
}

set default=0
set timeout=5
EOF

# Create GRUB stage files (requires grub-mkimage or similar)
# For simple case, we'll use grub-mkrescue if available
if command -v grub-mkrescue &> /dev/null; then
    echo -e "${BLUE}Building ISO with grub-mkrescue...${NC}"
    grub-mkrescue -o ../$OUTPUT_ISO $ISO_DIR 2>/dev/null || {
        echo -e "${BLUE}grub-mkrescue failed, using xorriso...${NC}"
        xorriso -as mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot \
            -boot-load-size 4 -boot-info-table -o ../$OUTPUT_ISO $ISO_DIR
    }
elif command -v xorriso &> /dev/null; then
    echo -e "${BLUE}Building ISO with xorriso...${NC}"
    xorriso -as mkisofs -R -J -V "ExoKernel" \
        -b boot.img -no-emul-boot -boot-load-size 4 \
        -o ../$OUTPUT_ISO $ISO_DIR
else
    echo -e "${RED}Error: Neither grub-mkrescue nor xorriso found${NC}"
    echo "Install grub2-tools or xorriso to continue"
    exit 1
fi

echo -e "${GREEN}✓ Build complete!${NC}"
echo -e "${GREEN}ISO image: ../$OUTPUT_ISO${NC}"
echo -e "${GREEN}Kernel binary: kernel.bin${NC}"
