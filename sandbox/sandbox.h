#ifndef __x86_64__
# error unsupported
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <seccomp.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <ftw.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>
#include <systemd/sd-daemon.h>
#include <systemd/sd-event.h>

#define POE_TEMPORARY_BASE "/tmp/poe"

#define POE_USERNAME "nobody"
#define POE_HOSTNAME "poe-sandbox"
#define POE_MEMORY_LIMIT (1024ULL * 1024ULL * 128ULL)
#define POE_TASKS_LIMIT 32ULL
#define POE_TIME_LIMIT (2ULL * 1000ULL * 1000ULL) // us

#define NONNEGATIVE(s) if ((s) < 0) ERROR("CRITICAL: %s:%d %s", __FILE__, __LINE__, #s)

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

void poe_init_seccomp(uint32_t);
void poe_init_systemd(pid_t);
void poe_exit_systemd(void);

char * poe_init_playground(const char *, const char *);
void poe_destroy_playground(void);
