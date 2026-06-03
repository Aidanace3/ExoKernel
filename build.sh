#!/bin/bash
set -e

# Configuration
CFLAGS="-m32 -ffreestanding -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-exceptions -c"
BIN_DIR="./bin"
OBJ_DIR="./bin/o"
KERNEL_OBJECTS="
    $OBJ_DIR/kernel_entry.o
    $OBJ_DIR/kernel.o
    $OBJ_DIR/shell.o
    $OBJ_DIR/libc.o
    $OBJ_DIR/pci.o
    $OBJ_DIR/vga.o
    $OBJ_DIR/ata.o
    $OBJ_DIR/fs.o
    $OBJ_DIR/fastfetch.o
    $OBJ_DIR/net.o
    $OBJ_DIR/e1000.o
"

mkdir -p "$BIN_DIR" "$OBJ_DIR"

echo "--- Compiling ---"
gcc $CFLAGS src/kernel.c -o $OBJ_DIR/kernel.o
gcc $CFLAGS src/shell.c -o $OBJ_DIR/shell.o
gcc $CFLAGS src/libc.c -o $OBJ_DIR/libc.o
gcc $CFLAGS src/pci.c -o $OBJ_DIR/pci.o
gcc $CFLAGS drivers/vga.c -o $OBJ_DIR/vga.o
gcc $CFLAGS drivers/ata.c -o $OBJ_DIR/ata.o
gcc $CFLAGS drivers/fs.c -o $OBJ_DIR/fs.o
gcc $CFLAGS src/fastfetch.c -o $OBJ_DIR/fastfetch.o
gcc $CFLAGS src/net.c -o $OBJ_DIR/net.o 
gcc $CFLAGS drivers/e1000.c -o $OBJ_DIR/e1000.o
nasm -f elf32 src/kernel_entry.asm -o $OBJ_DIR/kernel_entry.o
nasm -f bin -isrc/ src/boot.asm -o $BIN_DIR/boot.bin

echo "--- Linking ---"
ld -m elf_i386 -o $BIN_DIR/kernel.bin -Ttext 0x1000 $KERNEL_OBJECTS --oformat binary
ld -m elf_i386 -o $BIN_DIR/kernel.elf -T linker_grub.ld $KERNEL_OBJECTS

echo "--- Finalizing Image ---"
cat $BIN_DIR/boot.bin $BIN_DIR/kernel.bin > $BIN_DIR/exos_temp.img
dd if=/dev/zero of=$BIN_DIR/exos.img bs=512 count=45 >/dev/null 2>&1
dd if=$BIN_DIR/exos_temp.img of=$BIN_DIR/exos.img conv=notrunc >/dev/null 2>&1
rm $BIN_DIR/exos_temp.img

if [ ! -f "storage.img" ]; then
    dd if=/dev/zero of=storage.img bs=1M count=10 >/dev/null 2>&1
fi

if [ "$1" == "run" ]; then
    qemu-system-i386 -drive file=bin/exos.img,format=raw,index=0,media=disk \
                     -drive file=storage.img,format=raw,index=1,media=disk \
                     -netdev user,id=net0 -device e1000,netdev=net0
fi
