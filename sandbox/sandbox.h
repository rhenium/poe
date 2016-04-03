#ifndef SANDBOX_H
#define SANDBOX_H

#ifndef __x86_64__
# error unsupported
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
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
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

#define noreturn _Noreturn
#define unused __attribute__((__unused__))
#define warn_unused __attribute__((warn_unused_result))

enum poe_exit_reason {
	POE_SUCCESS,
	POE_SIGNALED,
	POE_TIMEDOUT,
};

struct playground {
	char *workbase;
	char *upperdir;
	char *workdir;
	char *mergeddir;
	char **command_line;
};

#include "config.h"
#include "utils.h"

// seccomp.c
warn_unused int poe_seccomp_init(void);
char *poe_seccomp_syscall_resolve(int);

// cgroup.c
warn_unused int poe_cgroup_init(void);
warn_unused int poe_cgroup_add(pid_t);

// playground.c
warn_unused struct playground *poe_playground_init(const char *, const char *);
warn_unused int poe_playground_init_command_line(struct playground *, char **, const char *);

noreturn void poe_child_do(struct playground *, int[2], int[2], int[2]);

#endif
