#ifndef _EDITOR_CONFIG_H
#define _EDITOR_CONFIG_H

#include "main.h"


struct abuf {
    char* b;
    int len;
};
#define ABUF_INIT {NULL, 0}
void ab_append(struct abuf* ab, char* s, int len);
void ab_free(struct abuf* ab);

typedef struct erow {
    int size;
    int rsize;
    char* chars;
    char* render;
} erow;

class EditorConfig
{
public:
    struct termios orig_termios;
    int screenrows;
    int screencols;
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int numrows;

    int dirty;

    char* filename;
    char statusmsg[80];
    time_t statusmsg_time;

    erow* row;
    
    EditorConfig();
    void editor_open(char* filename);
    void editor_append_row(int at, char* s, size_t len);
    void editor_update_row(erow* row);
    void editor_free_row(erow* row);
    int editor_row_cx_to_rx(erow* row, int cx);
    void editor_row_insert_char(erow* row, int at, int c);
    void editor_row_append_string(erow* row, char* s, size_t len);
    void editor_insert_newline();
    void editor_row_del_char(erow* row, int at);
    void editor_del_char();
    void editor_del_row(int at);
    void editor_insert_char(int c);
    void editor_scroll();
    int editor_read_key();
    int get_cursor_position(int rows, int cols);
    void editor_process_keypress();
    void editor_refresh_screen();
    void editor_draw_rows(struct abuf* ab);
    void editor_draw_status_bar(struct abuf* ab);
    void editor_draw_message_bar(struct abuf* ab);
    void editor_set_statusmsg(const char *fmt, ...);
    void editor_move_cursor(int key);
    char* editor_rows_to_string(int* bufflen);
    void editor_save();
    char* editor_prompt(char* prompt);
};

#endif // _EDITOR_CONFIG_H