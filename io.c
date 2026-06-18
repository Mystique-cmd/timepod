#include "io.h"

#include <stdio.h>
#include <stdlib.h>

void io_clear_screen(void) {
    /* ANSI clear screen */
    printf("\033[2J\033[H");
    fflush(stdout);
}

void io_print_domains(void) {
    printf("Select a domain to start a countdown:\n");
    printf("  1) The Portal (7 hours)\n");
    printf("  2) The Factory (4 hours)\n");
    printf("  3) Benjamin's Game (4 hours)\n");
    printf("  4) The Matrix Manual (1 hour)\n");
    printf("  5) The Rabbit Hole (4 hours)\n");
    printf("  6) Specter Spectacle (4 hours)\n");
    printf("  0) Exit\n");
}

static int read_int(void) {
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    return (int)strtol(buf, NULL, 10);
}

int io_read_int_choice(const char *prompt, int min_inclusive, int max_inclusive) {
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        int v = read_int();
        if (v >= min_inclusive && v <= max_inclusive) return v;
        printf("Invalid input. Enter a number between %d and %d.\n", min_inclusive, max_inclusive);
    }
}

