#include <stdint.h>

#include "sigio.h"

static int sigio_bit = SIGIO_NONE;

static void sigio_reset(void)
{
    sigio_bit = SIGIO_NONE;
}

static void sigio_wait_ack(void)
{
    int value;

    sigio_reset();
    do {
        pause();
        value = sigio_bit;
        sigio_reset();
    } while (value != SIGIO_TO_BIT(SIGIO_ACK));
}

static int sigio_wait_bit(void)
{
    int value;

    sigio_reset();
    do {
        pause();
        value = sigio_bit;
        sigio_reset();
    } while (value == SIGIO_NONE);
    return value;
}

static int sigio_send_ack(pid_t pid)
{
    return kill(pid, SIGIO_ACK);
}

static void sigio_handler(int sig)
{
    sigio_bit = SIGIO_TO_BIT(sig);
}

ssize_t sigio_write(pid_t pid, const void* data, size_t size)
{
    const int8_t* bits = data;
    int8_t bit;

    for (size_t i = 0; i < size * __CHAR_BIT__; ++i) {
        bit = (bits[i / 8] >> (i % 8)) & 1;
        if (kill(pid, BIT_TO_SIGIO(bit)) == -1) {
            sigio_reset();
            return -1;
        }
        sigio_wait_ack();
    }
    return size;
}

ssize_t sigio_read(pid_t pid, void* buffer, size_t size)
{
    int8_t* bits = buffer;
    int8_t bit;

    for (size_t i = 0; i < size * __CHAR_BIT__; ++i) {
        bit = sigio_wait_bit();
        if (bit == -1 || sigio_send_ack(pid) == -1) {
            return -1;
        }
        bits[i / 8] |= bit << (i % 8);
    }
    return size;
}

void sigio_setup(void)
{
    signal(SIGIO_ONE, &sigio_handler);
    signal(SIGIO_ZERO, &sigio_handler);
}

void sigio_fini(void)
{
    signal(SIGIO_ONE, SIG_DFL);
    signal(SIGIO_ZERO, SIG_DFL);
}