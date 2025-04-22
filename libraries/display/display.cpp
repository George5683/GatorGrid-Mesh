#include <cstdio>
#include "display.hpp"

void display_test() {
    printf( ANSI_CLEAR_SCROLLBACK ANSI_CLEAR_SCREEN ANSI_CURSOR_HOME );
    printf( ANSI_GREEN_TEXT "OK: all systems go!\r\n" ANSI_RESET );
    printf( ANSI_RED_TEXT   "ERR: sensor failure!\r\n" ANSI_RESET );
}