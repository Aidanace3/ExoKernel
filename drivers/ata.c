#include "../src/arch/io.h"

/* --- Fixed ATA PIO Disk Subsystem (LBA Mode - Slave Drive with Timeout) --- */
int ata_read_sector(unsigned int lba, unsigned char* buffer) {
    int timeout = 100000;
    while ((port_byte_in(0x1F7) & 0x80) && timeout > 0) { timeout--; }
    if (timeout <= 0) return 0; // Return 0 on failure

    // 0xF0 selects the Primary Slave Drive
    port_byte_out(0x1F6, 0xF0 | ((lba >> 24) & 0x0F));
    port_byte_out(0x1F2, 1);
    port_byte_out(0x1F3, (unsigned char) lba);
    port_byte_out(0x1F4, (unsigned char) (lba >> 8));
    port_byte_out(0x1F5, (unsigned char) (lba >> 16));
    port_byte_out(0x1F7, 0x20);

    timeout = 100000;
    while (!(port_byte_in(0x1F7) & 0x08) && timeout > 0) { timeout--; }
    if (timeout <= 0) return 0; // Return 0 on failure

    for (int i = 0; i < 256; i++) {
        unsigned short data = port_word_in(0x1F0);
        buffer[i * 2] = data & 0xFF;
        buffer[i * 2 + 1] = (data >> 8) & 0xFF;
    }
    return 1; // Return 1 on success
}

int ata_write_sector(unsigned int lba, unsigned char* buffer) {
    int timeout = 100000;
    while ((port_byte_in(0x1F7) & 0x80) && timeout > 0) { timeout--; }
    if (timeout <= 0) return 0; // Return 0 on failure

    // 0xF0 selects the Primary Slave Drive
    port_byte_out(0x1F6, 0xF0 | ((lba >> 24) & 0x0F));
    port_byte_out(0x1F2, 1);
    port_byte_out(0x1F3, (unsigned char) lba);
    port_byte_out(0x1F4, (unsigned char) (lba >> 8));
    port_byte_out(0x1F5, (unsigned char) (lba >> 16));
    port_byte_out(0x1F7, 0x30);

    timeout = 100000;
    while (!(port_byte_in(0x1F7) & 0x08) && timeout > 0) { timeout--; }
    if (timeout <= 0) return 0; // Return 0 on failure

    for (int i = 0; i < 256; i++) {
        unsigned short data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        port_word_out(0x1F0, data);
    }
    return 1; // Return 1 on success
}

