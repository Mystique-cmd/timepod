#ifndef TIMEPOD_TERMINAL_H
#define TIMEPOD_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
    bool configured;
} TerminalRawMode;

int terminal_enable_raw_mode(TerminalRawMode *st);
int terminal_disable_raw_mode(TerminalRawMode *st);

/* Get terminal size (rows/cols). Returns 0 on success, -1 on failure. */
int terminal_get_size(int *out_rows, int *out_cols);

/* Returns -1 if no key available. */
int terminal_read_key_nonblocking(void);


#ifdef __cplusplus
}
#endif

#endif

