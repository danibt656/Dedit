#include "../include/editorConfig.h"


void ab_append(struct abuf* ab, char* s, int len)
{
    char* newab = (char*)realloc(ab->b, ab->len + len);
    if (newab == NULL) return;
    memcpy(&newab[ab->len], s, len);
    ab->b = newab;
    ab->len += len;
}
void ab_free(struct abuf* ab)
{
    free(ab->b);
}


EditorConfig::EditorConfig()
{
    cx = 0;
    cy = 0;
    rx = 0;
    rowoff = 0;
    coloff = 0;
    numrows = 0;
    row = NULL;

    dirty = 0;

    filename = NULL;
    statusmsg[0] = '\0';
    statusmsg_time = 0;

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            die("ioctl write");
        this->get_cursor_position(screenrows, screencols);
    }

    screenrows = ws.ws_row;
    screencols = ws.ws_col;

    this->screenrows -= 2;
}

void EditorConfig::editor_open(char* filename)
{
    free(this->filename);
    this->filename = strdup(filename);

    FILE* fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen-1] == '\n' ||
                               line[linelen-1] == '\r'))
            linelen--;
        this->editor_append_row(this->numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    this->dirty = 0;
}

void EditorConfig::editor_append_row(int at, char* s, size_t len)
{
    if (at < 0 || at > this->numrows) return;
    this->row = (erow*)realloc(this->row, sizeof(erow)*(this->numrows + 1));
    memmove(&this->row[at+1], &this->row[at], sizeof(erow)*(this->numrows - at));

    this->row[at].size = len;
    this->row[at].chars = (char*)malloc(len + 1);
    memcpy(this->row[at].chars, s, len);
    this->row[at].chars[len] = '\0';

    this->row[at].rsize = 0;
    this->row[at].render = NULL;
    this->editor_update_row(&this->row[at]);

    this->numrows++;
    this->dirty++;
}

void EditorConfig::editor_update_row(erow* row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = (char*)malloc(row->size + tabs*(TAB_STOP-1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while(idx%TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void EditorConfig::editor_free_row(erow* row)
{
    free(row->render);
    free(row->chars);
}

int EditorConfig::editor_row_cx_to_rx(erow* row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        rx++;
    }
    return rx;
}

void EditorConfig::editor_row_insert_char(erow* row, int at, int c)
{
    if (at < 0 || at > row->size) at = row->size;
    row->chars = (char*)realloc(row->chars, row->size + 2);
    memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    this->editor_update_row(row);    
    this->dirty++;
}

void EditorConfig::editor_insert_char(int c)
{
    if (this->cy == this->numrows) {
        this->editor_append_row(this->numrows, "", 0);
    }
    this->editor_row_insert_char(&this->row[this->cy], this->cx, c);
    this->cx++;
}

void EditorConfig::editor_row_append_string(erow* row, char* s, size_t len)
{
    row->chars = (char*)realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    this->dirty++;
}

void EditorConfig::editor_insert_newline()
{
    if (this->cx == 0) {
        editor_append_row(this->cy, "", 0);
    } else {
        erow* row = &this->row[this->cy];
        editor_append_row(this->cy+1, &row->chars[this->cx], row->size - this->cx);
        row = &this->row[this->cy];
        row->size = this->cx;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }
    this->cy++;
    this->cx = 0;
}

void EditorConfig::editor_row_del_char(erow* row, int at)
{
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at+1], row->size - at);
    row->size--;
    this->editor_update_row(row);
    this->dirty++;  
}

void EditorConfig::editor_del_char()
{
    if (this->cy == this->numrows) return;
    if (this->cx == 0 && this->cy == 0) return;

    erow* row = &this->row[this->cy];
    if (this->cx > 0) {
        editor_row_del_char(row, this->cx - 1);
        this->cx--;
    } else {
        this->cx = this->row[this->cy-1].size;
        editor_row_append_string(&this->row[this->cy-1], row->chars, row->size);
        editor_del_row(this->cy);
        this->cy--;
    }
}

void EditorConfig::editor_del_row(int at)
{
    if (at < 0 || at >= this->numrows) return;
    editor_free_row(&this->row[at]);
    memmove(&this->row[at], &this->row[at+1], sizeof(erow)*(this->numrows - at - 1));
    this->numrows--;
    this->dirty++;
}

void EditorConfig::editor_scroll()
{
    this->rx = this->cx;
    if (this->cy < this->numrows)
        this->rx = this->editor_row_cx_to_rx(&this->row[this->cy], this->cx);

    if (this->cy < this->rowoff) {
        this->rowoff = this->cy;
    }
    if (this->cy >= this->rowoff + this->screenrows) {
        this->rowoff = this->cy - this->screenrows + 1;
    }
    if (this->cx < this->coloff) {
        this->coloff = this->rx;
    }
    if (this->cx >= this->coloff + this->screencols) {
        this->coloff = this->rx - this->screencols + 1;
    }
}

int EditorConfig::editor_read_key()
{
    int nread = 0;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return CURSOR_UP;
                    case 'B': return CURSOR_DOWN;
                    case 'C': return CURSOR_RIGHT;
                    case 'D': return CURSOR_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

int EditorConfig::get_cursor_position(int rows, int cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) die("write");;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
   
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) return -1;

    return 0;
}

void EditorConfig::editor_process_keypress()
{
    int c = this->editor_read_key();
    static int quit_times = QUIT_TIMES;

    switch (c) {
        case '\r':
            editor_insert_newline();
            break;

        case CTRL_KEY('q'):
            if (this->dirty && quit_times > 0) {
                this->editor_set_statusmsg("WARNING!!! File has %d unsaved changes. "
                "Press ^Q %d more times to quit", this->dirty, quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):
            this->editor_save();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    this->cy = this->rowoff;
                } else if (c = PAGE_DOWN) {
                    this->cy = this->rowoff + this->screenrows - 1;
                    if (this->cy > this->numrows) this->cy = this->numrows;
                }
                int times = this->screenrows;
                while (times--)
                    editor_move_cursor(c == PAGE_UP ? CURSOR_UP : CURSOR_DOWN);
            }
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editor_move_cursor(CURSOR_RIGHT);
            editor_del_char();
            break;

        case CURSOR_UP:
        case CURSOR_DOWN:
        case CURSOR_LEFT:
        case CURSOR_RIGHT:
            this->editor_move_cursor(c);
            break;

        case HOME_KEY:
            this->cx = 0;
            break;
        case END_KEY:
            if (this->cy < this->numrows)
                this->cx = this->row[this->cy].size;
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;
        
        default:
            editor_insert_char(c);
            break;
    }
    quit_times = QUIT_TIMES;
}

void EditorConfig::editor_refresh_screen()
{
    this->editor_scroll();

    struct abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    this->editor_draw_rows(&ab);
    this->editor_draw_status_bar(&ab);
    this->editor_draw_message_bar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (this->cy - this->rowoff) + 1,
                                              (this->rx - this->coloff) + 1);
    ab_append(&ab, buf, strlen(buf));

    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);
}

void EditorConfig::editor_draw_rows(struct abuf* ab)
{
    int y;
    for (y = 0; y < this->screenrows; y++) {
        int filerow = y + this->rowoff;
        if (filerow >= this->numrows) {
            if (this->numrows == 0 && y == this->screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), EDITOR_STATUS, EDITOR_VERSION);
                if (welcomelen > this->screencols) welcomelen = this->screencols;
                int padding = (this->screencols - welcomelen) / 2;
                if (padding) {
                    ab_append(ab, "~", 1);
                    padding--;
                }
                while (padding--) ab_append(ab, " ", 1);
                ab_append(ab, welcome, welcomelen);
            }
            else
                ab_append(ab, "~", 1);
        } else {
            int len = this->row[filerow].rsize - this->coloff;
            if (len < 0) len = 0;
            if (len > this->screencols) len = this->screencols;
            ab_append(ab, &this->row[filerow].render[this->coloff], len);
        }

        ab_append(ab, "\x1b[K", 3);
        ab_append(ab, "\r\n", 2);
    }
}

void EditorConfig::editor_draw_status_bar(struct abuf* ab)
{
    ab_append(ab, "\x1b[7m", 4);
    
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       this->filename ? this->filename : "[No Name]",
                       this->numrows,
                       this->dirty ? "(modified)" : ""
                       );
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", this->cy+1, this->numrows);
    if (len > this->screencols) len = this->screencols;
    ab_append(ab, status, len);

    while (len < this->screencols) {
        if (this->screencols - len == rlen) {
            ab_append(ab, rstatus, rlen);
            break;
        } else {
            ab_append(ab, " ", 1);
            len ++;
        }
    }
    ab_append(ab, "\x1b[m", 3);
    ab_append(ab, "\r\n", 2);
}

void EditorConfig::editor_draw_message_bar(struct abuf* ab)
{
    ab_append(ab, "\x1b[K", 3);
    int msglen = strlen(this->statusmsg);
    if (msglen > this->screencols) msglen = this->screencols;
    if (msglen && time(NULL) - this->statusmsg_time < 5)
        ab_append(ab, this->statusmsg, msglen);
}

void EditorConfig::editor_set_statusmsg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(this->statusmsg, sizeof(this->statusmsg), fmt, ap);
    va_end(ap);
    this->statusmsg_time = time(NULL);
}

void EditorConfig::editor_move_cursor(int key)
{
    erow* row = (this->cy >= this->numrows) ? NULL : &this->row[this->cy];

    switch (key)
    {
        case CURSOR_LEFT:
            if (this->cx !=0)
                this->cx--;
            else if (this->cy > 0) {
                this->cy--;
                this->cx = this->row[this->cy].size;
            }
            break;
        case CURSOR_RIGHT:
            if (row && this->cx < row->size)
                this->cx++;
            else if (row && this->cx == row->size) {
                this->cy++;
                this->cx = 0;
            }
            break;
        case CURSOR_UP:
            if (this->cy != 0)
                this->cy--;
            break;
        case CURSOR_DOWN:
            if (this->cy < this->numrows)
                this->cy++;
            break;
    }

    row = (this->cy > this->numrows) ? NULL : &this->row[this->cy];
    int rowlen = row ? row->size : 0;
    if (this->cx > rowlen) this->cx = rowlen;
}

char* EditorConfig::editor_rows_to_string(int* bufflen)
{
    int totlen = 0;
    int j;
    for (j = 0; j < this->numrows; j++)
        totlen += this->row[j].size + 1;
    *bufflen = totlen;

    char* buf = (char*)malloc(totlen);
    char* p = buf;
    for (j = 0; j < this->numrows; j++) {
        memcpy(p, this->row[j].chars, this->row[j].size);
        p += this->row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void EditorConfig::editor_save()
{
    if (this->filename == NULL) {
        this->filename = editor_prompt("Save as: %s");
        if (this->filename == NULL) {
            editor_set_statusmsg("Save aborted");
            return;
        }
    }

    int len;
    char* buf = editor_rows_to_string(&len);

    int fd = open(this->filename, O_RDWR | O_CREAT, STD_PRIVILEGES);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                this->dirty = 0;
                this->editor_set_statusmsg("%d bytes saved", len);
                return;
            }
        }
        close(fd);
    }
    free(buf);
    this->editor_set_statusmsg("Couldn't save! I/O error: %s", strerror(errno));
}

char* EditorConfig::editor_prompt(char* prompt)
{
    size_t bufsize = 128;
    char* buf = (char*)malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (true) {
        editor_set_statusmsg(prompt, buf);
        editor_refresh_screen();

        int c = editor_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        }else if (c == '\x1b') {
            editor_set_statusmsg("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editor_set_statusmsg("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = (char*)realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}