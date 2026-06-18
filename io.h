#ifndef TIMEPOD_IO_H
#define TIMEPOD_IO_H

#ifdef __cplusplus
extern "C" {
#endif

int io_read_int_choice(const char *prompt, int min_inclusive, int max_inclusive);
void io_clear_screen(void);
void io_print_domains(void);

#ifdef __cplusplus
}
#endif

#endif


