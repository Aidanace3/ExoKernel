#ifndef VGA_H
#define VGA_H

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_COLOR_WHITE_ON_BLACK 0x0F

void zeal_terminal_clear();
void zeal_putc(char c);
void zeal_write(const char* string);
void zeal_scroll_page_up();
void zeal_scroll_page_down();

#endif
