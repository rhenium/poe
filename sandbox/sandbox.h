#ifndef __x86_64__
# error unsupported
#endif

#include <stdio.h>
#include <stdlib.h>
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
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/signalfd.h>
#include <sys/reg.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEBUG true
#define POE_USERNAME "nobody"
#define POE_HOSTNAME "poe-sandbox"

#define POE_LOWERDIR "/"
#define POE_TEMPORARY_BASE "/tmp/poe"
#define POE_UPPERDIR_TEMPLATE POE_TEMPORARY_BASE "/upperXXXXXX"
#define POE_WORKDIR_TEMPLATE POE_TEMPORARY_BASE "/workXXXXXX"
#define POE_MERGEDDIR_TEMPLATE POE_TEMPORARY_BASE "/mergedXXXXXX"

#define POE_MEMORY_LIMIT (1024ULL * 1024ULL * 5ULL)
#define POE_TASKS_LIMIT 32ULL

#define ERROR(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    if (syscall(SYS_getpid) != 1) poe_destroy_playground();\
    exit(1);\
} while (false)

enum poe_handler_result {
    POE_PROHIBITED,
    POE_HANDLED,
    POE_ALLOWED
};

void poe_init_seccomp(uint32_t);

char * poe_init_playground(const char *, const char *);
void poe_destroy_playground();
