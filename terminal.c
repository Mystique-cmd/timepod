#include "terminal.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

static struct termios g_orig_termios;
static bool g_have_orig = false;

int terminal_enable_raw_mode(TerminalRawMode *st) {
    if (!st) return -1;

    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) {
        return -1;
    }
    g_have_orig = true;

    raw = g_orig_termios;
    /* input flags */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output flags */
    raw.c_oflag &= ~(OPOST);
    /* control flags */
    raw.c_cflag |= (CS8);
    /* local flags */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return -1;
    }

    /* non-blocking stdin */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    st->configured = true;
    return 0;
}

int terminal_disable_raw_mode(TerminalRawMode *st) {
    (void)st;
    if (g_have_orig) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
    }
    /* restore blocking mode */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }
    return 0;
}

int terminal_read_key_nonblocking(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return -1;
    return (int)c;
}

