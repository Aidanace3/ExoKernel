#define VGA_COLOR_DEFAULT 0x0F
extern int term_row; // This tells fastfetch.c "the variable exists somewhere else"

void zeal_write(const char* string);
void zeal_putc(char c);

extern char hostname[];
extern char current_user[];
extern int term_row; // Access global display tracking variables

// Function to call the CPUID opcode and read hardware registry arrays
void get_cpu_vendor(char* vendor_string) {
    unsigned int eax, ebx, ecx, edx;
    
    // Call cpuid with EAX=0 to get the vendor ID string
    eax = 0;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(eax));
    
    // The string is returned across 3 registers (12 characters total)
    *(unsigned int*)(&vendor_string[0]) = ebx;
    *(unsigned int*)(&vendor_string[4]) = edx;
    *(unsigned int*)(&vendor_string[8]) = ecx;
    vendor_string[12] = '\0';
}

// Simple integer-to-string utility because we don't have standard printf()
void zeal_itoa(int num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    // Process digits backwards
    int temp = num;
    while (temp > 0) {
        i++;
        temp /= 10;
    }
    str[i] = '\0';
    while (num > 0) {
        str[--i] = (num % 10) + '0';
        num /= 10;
    }
}

void print_fastfetch() {
    char cpu_name[13];
    get_cpu_vendor(cpu_name); // Query the actual virtual CPU
    
    char uptime_str[16];
    // Calculate a rough uptime step based on how many screen lines have rendered so far
    zeal_itoa(term_row * 3, uptime_str); 

    // Construct and stitch the real specs right next to the logo rows
    zeal_write("                       \n");
    zeal_write("   :::                 \n");
    zeal_write("  FYFY;       .aa.     \n");
    zeal_write(" nF  DD    ..d/'Fd     \n");
    zeal_write("    FMM    EE  'FFf    \n");
    zeal_write("     fDD  EE :,FF:     \n");
    zeal_write("     ffDaEE :MFF'      \n");
    zeal_write("      XKdaMMfH''       \n");
    
    zeal_write("    XRRRR              ");
    zeal_write(current_user); zeal_write("@"); zeal_write(hostname); zeal_write("\n");
    
    zeal_write("  KRRR                 -----------------\n");
    
    zeal_write("  MFEEEEFE             OS: ExOS v0.0.1 i386\n");
    
    zeal_write("   aXab_aF             Kernel: Pure 32-bit Protected Mode\n");
    
    zeal_write("        FY             Uptime: "); 
    zeal_write(uptime_str); zeal_write(" seconds\n");
    
    zeal_write("      FYY              Shell: sh 0.1\n");
    
    zeal_write("     YY                CPU: "); 
    zeal_write(cpu_name); zeal_write("\n"); // Will output "GenuineIntel" or "AuthenticAMD" depending on QEMU settings
    
    zeal_write("    FD                 Memory: Base 1MB / 128MB Map\n");
    zeal_write("   FFD                 \n");
    zeal_write("   FDDDD  FF           \n");
    zeal_write("   FFF:FFFF            \n");
    zeal_write("                       \n");
}
