#ifndef SANDBOX_H
#define SANDBOX_H

#ifndef __x86_64__
# error unsupported
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <getopt.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>
#include <ftw.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/reg.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

#define NONNEGATIVE(s) do if ((s) < 0) ERROR("CRITICAL: %s:%d %s", __FILE__, __LINE__, #s); while (0)
#define NONNULL(s) do if (!(s)) ERROR("CRITICAL: %s:%d %s", __FILE__, __LINE__, #s); while (0)
#define UNREACHABLE() __builtin_unreachable()

enum poe_exit_reason {
    POE_SUCCESS,
    POE_SIGNALED,
    POE_TIMEDOUT,
};

void ERROR(const char *, ...);
void FINISH(enum poe_exit_reason, int, const char *, ...);

enum poe_handler_result {
    POE_PROHIBITED,
    POE_HANDLED,
    POE_ALLOWED
};

struct syscall_rule {
    int syscall;
    uint32_t action;
};

void poe_seccomp_init(void);
void poe_seccomp_handle_syscall(pid_t, unsigned long);
void poe_init_systemd(pid_t);
void poe_exit_systemd(void);

char *poe_init_playground(const char *, const char *);
char *poe_copy_program_to_playground(const char *, const char *);
void poe_destroy_playground(void);

static void __attribute__ ((unused))
print_backtrace(void)
{
    void *trace[128];
    int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
    backtrace_symbols_fd(trace, n, 2);
}

#include "config.h"
#endif
