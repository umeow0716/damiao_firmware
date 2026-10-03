#ifndef DAMIAO_DEBUG_CONSOLE_H
#define DAMIAO_DEBUG_CONSOLE_H

#include <stddef.h>
#include <stdint.h>

void debug_console_reset(void);
void debug_console_receive(uint8_t byte);
void debug_console_end_frame(void);
void debug_console_receive_frame(const uint8_t *data, size_t length);
void debug_console_process_dma_frame(const uint8_t *data, int16_t length);
void debug_console_return_to_menu(void);
int debug_console_printf(const char *format, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2)));
#else
    ;
#endif
void debug_console_print_banner(void);
void debug_console_print_status(void);

#endif
