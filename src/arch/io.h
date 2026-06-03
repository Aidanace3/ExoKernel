#ifndef ARCH_IO_H
#define ARCH_IO_H

static inline unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

static inline void port_byte_out(unsigned short port, unsigned char data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

static inline unsigned short port_word_in(unsigned short port) {
    unsigned short result;
    __asm__("inw %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

static inline void port_word_out(unsigned short port, unsigned short data) {
    __asm__("outw %%ax, %%dx" : : "a" (data), "d" (port));
}
static inline void outl(unsigned short port, unsigned int data) {
    __asm__("outl %0, %1" : : "a"(data), "Nd"(port));
}

static inline unsigned int inl(unsigned short port) {
    unsigned int result;
    __asm__("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

#endif
