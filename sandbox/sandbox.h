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
#include <limits.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/reg.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

#define UNREACHABLE() __builtin_unreachable()
#define C_SYSCALL(s) if ((s) < 0) ERROR("CRITICAL: %s:%d %s", __FILE__, __LINE__, #s)

enum poe_exit_reason {
    POE_SUCCESS,
    POE_SIGNALED,
    POE_TIMEDOUT,
};

static void
ERROR(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

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
char *poe_seccomp_handle_syscall(pid_t, unsigned long);
void poe_cgroup_init(void);
void poe_cgroup_add(pid_t);
void poe_cgroup_destroy(void);
void poe_cgroup_unmount(void);

char *poe_init_playground(const char *, const char *);
char *poe_copy_program_to_playground(const char *, const char *);
void poe_destroy_playground(void);

void poe_do_child(const char *, char **, int);

static void __attribute__ ((unused))
print_backtrace(void)
{
    void *trace[128];
    int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
    backtrace_symbols_fd(trace, n, 2);
}

#include "config.h"
#endif
