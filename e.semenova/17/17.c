#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <termios.h>

#define MAXLEN 40
#define CTRL_D 0x04
#define CTRL_W 0x17
#define ESC 0x1B

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0
#endif

static struct termios orig;
static char buf[MAXLEN];
static int len = 0;

static void out(const char *s, size_t n)
{
    ssize_t r = write(STDOUT_FILENO, s, n);
    (void)r;
}

static void beep(void)
{
    out("\a", 1);
}

static void erase_one(void)
{
    out("\b \b", 3);
}

static void newline(void)
{
    out("\n", 1);
    len = 0;
}

static void on_signal(int sig)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    _exit(128 + sig);
}

static int read_byte_timeout(unsigned char *c, int ms)
{
    struct pollfd p;
    int r;

    p.fd = STDIN_FILENO;
    p.events = POLLIN;
    do {
        r = poll(&p, 1, ms);
    } while (r == -1 && errno == EINTR);
    if (r <= 0)
        return 0;
    return read(STDIN_FILENO, c, 1) == 1;
}

static void skip_escape(void)
{
    unsigned char c;

    if (!read_byte_timeout(&c, 50))
        return;
    if (c == '[' || c == 'O') {
        while (read_byte_timeout(&c, 50)) {
            if (c >= 0x40 && c <= 0x7E)
                break;
        }
    }
}

static void put_char(char c)
{
    char word[MAXLEN];
    int start, n, i;

    if (len == MAXLEN) {
        if (c == ' ') {
            newline();
            return;
        }
        start = len;
        while (start > 0 && buf[start - 1] != ' ')
            start--;
        if (start > 0 && start < len) {
            n = len - start;
            memcpy(word, buf + start, n);
            for (i = 0; i < n; i++)
                erase_one();
            newline();
            memcpy(buf, word, n);
            len = n;
            out(word, n);
        } else {
            newline();
        }
    }
    buf[len++] = c;
    out(&c, 1);
}

static void erase_char(void)
{
    if (len > 0) {
        len--;
        erase_one();
    }
}

static void kill_line(void)
{
    while (len > 0) {
        len--;
        erase_one();
    }
}

static void erase_word(void)
{
    while (len > 0 && buf[len - 1] == ' ') {
        len--;
        erase_one();
    }
    while (len > 0 && buf[len - 1] != ' ') {
        len--;
        erase_one();
    }
}

int main(void)
{
    struct termios raw;
    unsigned char c;
    unsigned char erase_ch, kill_ch;
    ssize_t r;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: input must be a terminal\n");
        return 1;
    }

    if (tcgetattr(STDIN_FILENO, &orig) == -1) {
        perror("tcgetattr error");
        return 1;
    }

    erase_ch = orig.c_cc[VERASE];
    kill_ch = orig.c_cc[VKILL];

    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    signal(SIGINT, on_signal);
    signal(SIGQUIT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP, on_signal);
    signal(SIGTSTP, SIG_IGN);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
        perror("tcsetattr error");
        return 1;
    }

    for (;;) {
        r = read(STDIN_FILENO, &c, 1);
        if (r == -1) {
            if (errno == EINTR)
                continue;
            perror("read error");
            tcsetattr(STDIN_FILENO, TCSANOW, &orig);
            return 1;
        }
        if (r == 0)
            break;

        if (erase_ch != _POSIX_VDISABLE && c == erase_ch) {
            erase_char();
        } else if (kill_ch != _POSIX_VDISABLE && c == kill_ch) {
            kill_line();
        } else if (c == CTRL_W) {
            erase_word();
        } else if (c == CTRL_D) {
            if (len == 0)
                break;
            beep();
        } else if (c == '\n' || c == '\r') {
            newline();
        } else if (c == ESC) {
            beep();
            skip_escape();
        } else if (isprint(c)) {
            put_char((char)c);
        } else {
            beep();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    return 0;
}
