#include <cstdint>

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
#define ANSI_GRAY_TEXT   "\x1B[90m"



#if DEBUG
  #define DEBUG_printf(fmt, ...) do {                                        \
    printf(ANSI_GRAY_TEXT "[DEBUG] %s: ", __func__);                         \
    printf(fmt, ##__VA_ARGS__);                                              \
    printf(ANSI_RESET "\n");                                                 \
  } while (0)

  #define ERROR_printf(fmt, ...) do {                                        \
    printf(ANSI_RED_TEXT "[ERROR] %s: ", __func__);                          \
    printf(fmt, ##__VA_ARGS__);                                              \
    printf(ANSI_RESET "\n");                                                 \
  } while (0)
#else
  #define DEBUG_printf(...) do{}while(0)
  #define ERROR_printf(...) do{}while(0)
#endif

#if DEBUG
 static void dump_bytes(const uint8_t *bptr, uint32_t len) {
     unsigned int i = 0;
 
    printf(ANSI_GRAY_TEXT  "dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
            printf("\n");
         } else if ((i & 0x07) == 0) {
            printf(" ");
         }
        printf("%02x ", bptr[i++]);
     }
    printf(ANSI_RESET "\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif

void display_test();