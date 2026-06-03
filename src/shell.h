#ifndef SHELL_H
#define SHELL_H

// Declare these so shell.c knows they exist
void print_prompt();
void execute_command();
void check_keyboard_input(); 
void exaget(char* ip, char* path);
void wifictl(char* args);

// Add these declarations:
int zeal_strcmp(char *s1, char *s2);

#endif
