#include <signal.h>
#include <unistd.h>

#define SIGIO_ZERO SIGUSR1
#define SIGIO_ONE SIGUSR2
#define SIGIO_ACK SIGIO_ONE
#define SIGIO_NONE -1
#define SIGIO_TO_BIT(SIG) (SIG == SIGIO_ONE ? 1 : 0)
#define BIT_TO_SIGIO(BIT) (BIT == 1 ? SIGIO_ONE : SIGIO_ZERO)

ssize_t sigio_write(pid_t pid, const void *data, size_t size);

ssize_t sigio_pread(pid_t *pid, void *buffer, size_t size);
ssize_t sigio_read(pid_t pid, void *buffer, size_t size);

void sigio_setup(void);

void sigio_fini(void);