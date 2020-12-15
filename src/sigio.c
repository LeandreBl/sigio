#include <stdint.h>

#include "sigio.h"

static int __sigio_bit = SIGIO_NONE;
static pid_t __sigio_remote_pid = SIGIO_NONE;

static void sigio_reset(void)
{
    __sigio_bit = SIGIO_NONE;
}

static void sigio_wait_ack(void)
{
    int value;

    sigio_reset();
    do {
        pause();
        value = __sigio_bit;
        sigio_reset();
    } while (value != SIGIO_TO_BIT(SIGIO_ACK));
}

static int sigio_wait_bit(int *pid)
{
    int value;

    sigio_reset();
    do {
        pause();
        if (*pid == -1) {
            *pid = __sigio_remote_pid;
        } else if (__sigio_remote_pid != *pid) {
            return -1;
        }
        value = __sigio_bit;
        sigio_reset();
    } while (value == SIGIO_NONE);
    return value;
}

static int sigio_send_ack(pid_t pid)
{
    return kill(pid, SIGIO_ACK);
}

static void sigio_handler(int sig, siginfo_t *info, void *ucontext)
{
    (void)ucontext;
    __sigio_bit = SIGIO_TO_BIT(sig);
    __sigio_remote_pid = info->si_pid;
}

ssize_t sigio_write(pid_t pid, const void *data, size_t size)
{
    const int8_t *bits = data;
    int8_t bit;

    for (size_t i = 0; i < size * __CHAR_BIT__; ++i) {
        bit = (bits[i / 8] >> (i % 8)) & 1UL;
        if (kill(pid, BIT_TO_SIGIO(bit)) == -1) {
            sigio_reset();
            return -1;
        }
        sigio_wait_ack();
    }
    return size;
}

ssize_t sigio_read(pid_t pid, void *buffer, size_t size)
{
    int8_t *bits = buffer;
    int8_t bit;

    for (size_t i = 0; i < size * __CHAR_BIT__; ++i) {
        bit = sigio_wait_bit(&pid);
        if (bit == -1 || sigio_send_ack(pid) == -1) {
            return -1;
        }
        bits[i / 8] = (bits[i / 8] & ~(1UL << (i % 8))) | (bit << (i % 8));
    }
    return size;
}

ssize_t sigio_pread(pid_t *pid, void *buffer, size_t size)
{
    pid_t rpid = -1;
    int8_t *bits = buffer;
    int8_t bit;

    for (size_t i = 0; i < size * __CHAR_BIT__; ++i) {
        bit = sigio_wait_bit(&rpid);
        if (bit == -1 || sigio_send_ack(rpid) == -1) {
            return -1;
        }
        bits[i / 8] = (bits[i / 8] & ~(1UL << (i % 8))) | (bit << (i % 8));
    }
    if (pid != NULL) {
        *pid = rpid;
    }
    return size;
}

void sigio_setup(void)
{
    const struct sigaction cfg = {
        .sa_mask = { { SA_NODEFER } },
        .sa_sigaction = &sigio_handler,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGIO_ONE, &cfg, NULL);
    sigaction(SIGIO_ZERO, &cfg, NULL);
}

void sigio_fini(void)
{
    signal(SIGIO_ONE, SIG_DFL);
    signal(SIGIO_ZERO, SIG_DFL);
}