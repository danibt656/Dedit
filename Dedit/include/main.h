#ifndef _MAIN_H
#define _MAIN_H

#include <iostream>

extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <errno.h>
    #include <unistd.h>
    #include <string.h>
    #include <termios.h>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <time.h>
    #include <stdarg.h>
    #include <fcntl.h>
}

#define EDITOR_VERSION "0.1"
#define EDITOR_STATUS "Dedit editor -- version %s"

#define CTRL_KEY(k) ((k) & 0x1f)

#define BACKSPACE 127
#define CURSOR_LEFT 1000
#define CURSOR_RIGHT 1001
#define CURSOR_UP 1002
#define CURSOR_DOWN 1003
#define PAGE_UP 1004
#define PAGE_DOWN 1005
#define HOME_KEY 1006
#define END_KEY 1007
#define DEL_KEY 1008

#define TAB_STOP 8
#define STD_PRIVILEGES 0644
#define QUIT_TIMES 3

void die(const char *s);

void disableRawMode();

void enableRawMode();

char editor_read_key();

void editor_process_keypress();

void editor_refresh_screen();

void editor_draw_rows();

#endif // _MAIN_H