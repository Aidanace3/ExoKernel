#include "arch/io.h"
#include "../drivers/vga.h" // For logging to screen

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return (unsigned short)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
}

void probe_pci() {
    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned short slot = 0; slot < 32; slot++) {
            unsigned short vendor = pci_config_read_word(bus, slot, 0, 0);
            if (vendor == 0x8086) { // Intel
                unsigned short device = pci_config_read_word(bus, slot, 0, 2);
                if (device == 0x100E) {
                    zeal_write("Found Intel E1000 Network Card!\n");
                    // Future: Map BAR0 memory address here
                    return;
                }
            }
        }
    }
}
