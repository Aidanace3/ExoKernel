#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/ata.h"
#include "../drivers/fs.h"
#include "arch/io.h"

/* --- Global Variables --- */
char hostname[32] = "exos";
char current_user[32] = "root";
int term_row = 0; // Defined here to resolve the linker error

/* --- External Declarations --- */
extern void probe_pci(); // Declare the function from pci.c

void main() {
    /* 1. Initialize Display */
    zeal_terminal_clear();
    
    /* 2. Welcome Message */
    zeal_write("ExOS v0.0.1\n");
    zeal_write("----------------------------------------\n");
    
    /* 3. Hardware Detection */
    zeal_write("Probing PCI bus...\n");
    probe_pci(); // This will print "Found Intel E1000!" if successful

    /* 4. Filesystem */
    init_fs();
    
    /* 5. Start Shell */
    print_prompt(); 

    /* 6. Main Loop */
    while(1) {
        check_keyboard_input(); 
    }
}
