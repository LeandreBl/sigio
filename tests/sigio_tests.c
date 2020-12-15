#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sigio.h"

#define TIMER_FOR_LISTENER 10000

static void single_byte(void)
{
    pid_t parent = getpid();
    pid_t child = fork();
    ssize_t n;
    char c;

    if (child == 0) {
        n = sigio_read(parent, &c, sizeof(c));
        assert(n == 1);
        assert(c == 5);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        c = 5;
        n = sigio_write(child, &c, sizeof(c));
        assert(n == 1);
    }
}

static void no_byte(void)
{
    pid_t parent = getpid();
    pid_t child = fork();
    ssize_t n;
    char c = 18;

    if (child == 0) {
        n = sigio_read(parent, &c, 0);
        assert(n == 0);
        assert(c == 18);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        c = 5;
        n = sigio_write(child, NULL, 0);
        assert(n == 0);
    }
}

static void whole_string(void)
{
    pid_t parent = getpid();
    pid_t child = fork();
    ssize_t n;
    const char string[] = "Hi, I'm having fun with signals";
    char buffer[sizeof(string)] = { 0 };

    if (child == 0) {
        n = sigio_read(parent, buffer, sizeof(string));
        assert(n == sizeof(string));
        assert(strcmp(string, buffer) == 0);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        n = sigio_write(child, string, sizeof(string));
        assert(n == sizeof(string));
    }
}

struct test_s {
    int a;
    double b;
    char c;
} __attribute__((packed));

static void structure(void)
{
    pid_t parent = getpid();
    pid_t child = fork();
    ssize_t n;
    const struct test_s ref = { .a = 5, .b = 3.14, .c = '\b' };
    struct test_s st = { 0 };

    if (child == 0) {
        n = sigio_read(parent, &st, sizeof(st));
        assert(n == sizeof(st));
        assert(memcmp(&ref, &st, sizeof(st)) == 0);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        n = sigio_write(child, &ref, sizeof(ref));
        assert(n == sizeof(ref));
    }
}

static void with_get_pid(void)
{
    pid_t parent = -1;
    pid_t ref_parent = getpid();
    pid_t child = fork();
    ssize_t n;
    const int ref = 5;
    int v;

    if (child == 0) {
        n = sigio_pread(&parent, &v, sizeof(ref));
        assert(n == sizeof(ref));
        assert(ref == v);
        assert(parent == ref_parent);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        n = sigio_write(child, &ref, sizeof(ref));
        assert(n == sizeof(ref));
    }
}

static void with_null_pid(void)
{
    pid_t child = fork();
    ssize_t n;
    const int ref = 5;
    int v;

    if (child == 0) {
        n = sigio_pread(NULL, &v, sizeof(ref));
        assert(n == sizeof(ref));
        assert(ref == v);
        sigio_fini();
        exit(0);
    } else {
        usleep(TIMER_FOR_LISTENER);
        n = sigio_write(child, &ref, sizeof(ref));
        assert(n == sizeof(ref));
    }
}

static void (*TESTS_FUNCTIONS[])(void)
    = { &structure,    &single_byte,  &no_byte,
        &whole_string, &with_get_pid, &with_null_pid };

/* Custom main because Criterion does not handle what I want */
int main(void)
{
    sigio_setup();
    for (size_t i = 0; i < sizeof(TESTS_FUNCTIONS) / sizeof(*TESTS_FUNCTIONS);
         ++i) {
        TESTS_FUNCTIONS[i]();
        usleep(TIMER_FOR_LISTENER * 10);
    }
    sigio_fini();
}