#include "vga.h"

#define VGA_SCROLLBACK_ROWS 200
#define VGA_SCROLL_STEP (VGA_ROWS - 1)

/* --- Private VGA State --- */
volatile char* video_memory = (volatile char*) 0xb8000;
static char terminal_buffer[VGA_SCROLLBACK_ROWS][VGA_COLS];
static int cursor_col = 0;
static int cursor_row = 0;
static int viewport_row = 0;

/* --- Externs for boundary checking (defined in kernel.c) --- */
extern char hostname[32];
extern char current_user[32];
extern int term_row;

static void clear_buffer() {
    for (int row = 0; row < VGA_SCROLLBACK_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            terminal_buffer[row][col] = ' ';
        }
    }
}

static int bottom_viewport() {
    int bottom = cursor_row - VGA_ROWS + 1;
    return bottom > 0 ? bottom : 0;
}

static void render_viewport() {
    for (int row = 0; row < VGA_ROWS; row++) {
        int source_row = viewport_row + row;

        for (int col = 0; col < VGA_COLS; col++) {
            int index = (row * VGA_COLS + col) * 2;
            video_memory[index] = terminal_buffer[source_row][col];
            video_memory[index + 1] = VGA_COLOR_WHITE_ON_BLACK;
        }
    }
}

static void clear_line(int row) {
    for (int col = 0; col < VGA_COLS; col++) {
        terminal_buffer[row][col] = ' ';
    }
}

static void shift_scrollback() {
    for (int row = 1; row < VGA_SCROLLBACK_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            terminal_buffer[row - 1][col] = terminal_buffer[row][col];
        }
    }

    clear_line(VGA_SCROLLBACK_ROWS - 1);
    cursor_row = VGA_SCROLLBACK_ROWS - 1;
    if (viewport_row > 0) viewport_row--;
}

static void advance_line() {
    cursor_col = 0;
    cursor_row++;

    if (cursor_row >= VGA_SCROLLBACK_ROWS) {
        shift_scrollback();
    }

    clear_line(cursor_row);
    viewport_row = bottom_viewport();
    term_row = cursor_row;
}

void zeal_terminal_clear() {
    clear_buffer();
    cursor_col = 0;
    cursor_row = 0;
    viewport_row = 0;
    term_row = 0;
    render_viewport();
}

void zeal_scroll_page_up() {
    viewport_row -= VGA_SCROLL_STEP;
    if (viewport_row < 0) viewport_row = 0;
    render_viewport();
}

void zeal_scroll_page_down() {
    int bottom = bottom_viewport();

    viewport_row += VGA_SCROLL_STEP;
    if (viewport_row > bottom) viewport_row = bottom;
    render_viewport();
}

void zeal_putc(char c) {
    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            terminal_buffer[cursor_row][cursor_col] = ' ';
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_COLS - 1;
            terminal_buffer[cursor_row][cursor_col] = ' ';
        }

        viewport_row = bottom_viewport();
        term_row = cursor_row;
        render_viewport();
        return;
    }

    if (c == '\n') {
        advance_line();
        render_viewport();
        return;
    }

    terminal_buffer[cursor_row][cursor_col] = c;
    cursor_col++;

    if (cursor_col >= VGA_COLS) {
        advance_line();
    } else {
        viewport_row = bottom_viewport();
        term_row = cursor_row;
    }

    render_viewport();
}

void zeal_write(const char* string) {
    int i = 0;
    while (string[i] != '\0') {
        zeal_putc(string[i]);
        i++;
    }
}
