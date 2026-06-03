#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/ata.h"
#include "../drivers/fs.h"
#include "arch/io.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

/* --- Shell State (Private to this module) --- */
static char cmd_buffer[128];
static int cmd_index = 0;
static int shift_pressed = 0;
static unsigned char last_scancode = 0;
static int sh_depth = 1;
static int extended_scancode = 0;

/* --- Scancode Tables --- */
static const char scancode_to_ascii_lowercase[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

static const char scancode_to_ascii_uppercase[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

/* --- External Dependencies --- */
extern char hostname[32];
extern char current_user[32];
extern void print_fastfetch();
extern void exaget(char* ip, char* path);
extern void wifictl(char* args);

static char* skip_spaces(char* s) {
    while (s && *s == ' ') s++;
    return s;
}

static char* split_arg(char* s) {
    if (!s) return NULL;

    while (*s != '\0') {
        if (*s == ' ') {
            *s = '\0';
            return skip_spaces(s + 1);
        }
        s++;
    }

    return NULL;
}

static void print_help() {
    zeal_write("Commands:\n");
    zeal_write("  sh, exit, clear, fastfetch, uname, help\n");
    zeal_write("  ls, cat <file>, touch <file>, write <file> <text>, rm <file>\n");
    zeal_write("  exaget <path>, wifictl [status|scan|connect <ssid>|disconnect|on|off]\n");
    zeal_write("Keys: PageUp/PageDown scroll terminal history\n");
}

/* --- Implementation --- */
void print_prompt() {
    if (sh_depth > 1) {
        zeal_write("sh-0.1# ");
        return;
    }

    zeal_putc('[');
    zeal_write(current_user);
    zeal_putc('@');
    zeal_write(hostname);
    if (zeal_strcmp(current_user, "root") == 0) {
        zeal_write(":~]# ");
    } else {
        zeal_write(":~]$ ");
    }
}

void execute_command() {
    zeal_putc('\n');
    cmd_buffer[cmd_index] = '\0'; // Ensure buffer is terminated

    char *cmd = cmd_buffer;
    char *args = NULL;

    // Tokenization: Find the first space to separate command and arguments
    for (int i = 0; i < cmd_index; i++) {
        if (cmd_buffer[i] == ' ') {
            cmd_buffer[i] = '\0';      // End the command string
            args = &cmd_buffer[i + 1]; // Pointer to the start of arguments
            break;
        }
    }

    if (cmd_index > 0) {
        // --- Command Execution Logic ---
        if (zeal_strcmp(cmd, "clear") == 0) {
            zeal_terminal_clear();
        } 
        else if (zeal_strcmp(cmd, "sh") == 0) {
            sh_depth++;
            zeal_write("sh: ExOS shell 0.1\n");
        }
        else if (zeal_strcmp(cmd, "exit") == 0) {
            if (sh_depth > 1) {
                sh_depth--;
                zeal_write("exit\n");
            } else {
                zeal_write("sh: cannot exit initial shell\n");
            }
        }
        else if (zeal_strcmp(cmd, "fastfetch") == 0) {
            print_fastfetch();
        } 
        else if (zeal_strcmp(cmd, "uname") == 0) {
            zeal_write("ExOS 0.0.1 i386 HobbyOS\n");
        }
        else if (zeal_strcmp(cmd, "help") == 0) {
            print_help();
        } 
        else if (zeal_strcmp(cmd, "ls") == 0) {
            list_files();
        }
        else if (zeal_strcmp(cmd, "cat") == 0) {
            fs_cat(args);
        }
        else if (zeal_strcmp(cmd, "touch") == 0) {
            fs_touch(args);
        }
        else if (zeal_strcmp(cmd, "write") == 0) {
            char* text = split_arg(args);
            fs_write(args, text);
        }
        else if (zeal_strcmp(cmd, "rm") == 0) {
            fs_delete(args);
        }
        else if (zeal_strcmp(cmd, "exaget") == 0) {
            if (args != NULL) {
                exaget("10.0.2.2", args);
            } else {
                zeal_write("Usage: exaget <path>\n");
            }
        }
        else if (zeal_strcmp(cmd, "wifictl") == 0) {
            wifictl(args);
        } 
        else {
            zeal_write("sh: command not found: ");
            zeal_write(cmd);
            zeal_putc('\n');
        }
    }
    
    cmd_index = 0; // Reset for next input
    print_prompt();
}
void check_keyboard_input() {
    if (port_byte_in(0x64) & 1) {
        unsigned char scancode = port_byte_in(0x60);

        if (scancode == 0xE0) {
            extended_scancode = 1;
            return;
        }

        if (extended_scancode) {
            extended_scancode = 0;
            if (scancode == 0x49) {
                zeal_scroll_page_up();
            } else if (scancode == 0x51) {
                zeal_scroll_page_down();
            }
            return;
        }

        if (scancode == last_scancode) return;
        last_scancode = scancode;

        if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
        if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; return; }

        if (scancode < 128) {
            char c = shift_pressed ? scancode_to_ascii_uppercase[scancode] 
                                   : scancode_to_ascii_lowercase[scancode];
            if (c != 0) {
                if (c == '\n') execute_command();
                else if (c == '\b') {
                    if (cmd_index > 0) { cmd_index--; zeal_putc('\b'); }
                } else if (cmd_index < 127) {
                    cmd_buffer[cmd_index++] = c;
                    zeal_putc(c);
                }
            }
        }
    } else {
        last_scancode = 0;
    }
}
