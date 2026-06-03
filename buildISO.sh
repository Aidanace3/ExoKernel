echo "--- Creating ISO Image ---"
ISO_DIR="./iso_root"
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# Verify the GRUB kernel exists before copying
if [ -f "./bin/kernel.elf" ]; then
    cp "./bin/kernel.elf" "$ISO_DIR/boot/exos.elf"
else
    echo "Error: ./bin/kernel.elf not found. Run ./build.sh first."
    exit 1
fi

# Create GRUB configuration
cat <<EOF > "$ISO_DIR/boot/grub/grub.cfg"
menuentry "ExOS" {
    multiboot /boot/exos.elf
}
EOF

# Build the ISO
grub-mkrescue -o "exos.iso" "$ISO_DIR"

# Cleanup
rm -rf "$ISO_DIR"

if [ "$1" == "run" ]; then
    qemu-system-i386 -cdrom "exos.iso" \
                     -drive file=storage.img,format=raw,index=1,media=disk \
                     -netdev user,id=net0 -device e1000,netdev=net0
fi
