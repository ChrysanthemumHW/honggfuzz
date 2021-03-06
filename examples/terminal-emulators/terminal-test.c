#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

#define ARRAYSIZE(x) (sizeof(x) / sizeof(*x))

static int fd_tty;
static int fd_log;

int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    fd_tty = open("/dev/tty", O_RDWR | O_NONBLOCK);
    if (fd_tty == -1) {
        perror("open('/dev/tty')");
        exit(EXIT_FAILURE);
    }
    fd_log = open("./term.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_log == -1) {
        perror("open('./term.log')");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static bool isInteresting(const char* s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '[') {
            continue;
        }
        if (s[i] == ']') {
            continue;
        }
        if (s[i] == '?') {
            continue;
        }
        if (s[i] == ';') {
            continue;
        }
        if (s[i] == 'c') {
            continue;
        }
        if (s[i] == 'R') {
            continue;
        }
        if (s[i] == '\0') {
            continue;
        }
        if (isdigit(s[i])) {
            continue;
        }
        return true;
    }
    return false;
}

int LLVMFuzzerTestOneInput(uint8_t* buf, size_t len)
{
    write(fd_tty, buf, len);

    char read_buf[1024 * 1024];
    ssize_t sz = read(fd_tty, read_buf, sizeof(read_buf));

    if (sz > 0 && isInteresting(read_buf, sz)) {
        static const char msg_in[] = "\n============ IN ============\n";
        static const char msg_out[] = "\n============ OUT ===========\n";
        static const char msg_end[] = "\n============================\n";

        struct iovec iov[] = {
            {
                .iov_base = (void*)msg_in,
                .iov_len = sizeof(msg_in),
            },
            {
                .iov_base = (void*)buf,
                .iov_len = len,
            },
            {
                .iov_base = (void*)msg_out,
                .iov_len = sizeof(msg_out),
            },
            {
                .iov_base = (void*)read_buf,
                .iov_len = sz,
            },
            {
                .iov_base = (void*)msg_end,
                .iov_len = sizeof(msg_end),
            },
        };

        writev(fd_log, iov, ARRAYSIZE(iov));
    }

    return 0;
}
