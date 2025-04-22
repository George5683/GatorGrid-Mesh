#define ANSI_CLEAR_SCREEN  "\x1B[2J"   // Erase entire screen
#define ANSI_CURSOR_HOME   "\x1B[H"    // Move cursor to upper‑left
#define ANSI_CLEAR_SCROLLBACK  "\x1B[3J"
#define ANSI_RESET       "\x1B[0m"
#define ANSI_BOLD        "\x1B[1m"
#define ANSI_UNDERLINE   "\x1B[4m"

#define ANSI_RED_TEXT    "\x1B[31m"
#define ANSI_GREEN_TEXT  "\x1B[32m"
#define ANSI_YELLOW_TEXT "\x1B[33m"
#define ANSI_BLUE_TEXT   "\x1B[34m"

void display_test();