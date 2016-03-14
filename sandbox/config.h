#ifndef CONFIG_H
#define CONFIG_H

#define POE_TEMPORARY_BASE "/work/poe/sandbox/tmp"
#define POE_USERNAME "nobody"
#define POE_HOSTNAME "poe-sandbox"
#define POE_MEMORY_LIMIT (1024ULL * 1024ULL * 128ULL)
#define POE_TASKS_LIMIT 32ULL
#define POE_TIME_LIMIT (1000ULL * 1000ULL * 5ULL * 100ULL) // us
#define POE_CGROUP_ROOT POE_TEMPORARY_BASE"/cgroup"
#define POE_UID 27627
#define POE_GID 27627

// syscall filter
#include <seccomp.h>
#define RULE(name, action) { SCMP_SYS(name), SCMP_ACT_##action }
#define ALLOW(name) RULE(name, ALLOW)
/* because of Linux 4.4 bug, we can't kill stopped init. so kill without stopping it
#define KILL(name) RULE(name, TRACE(2)) */
#define KILL(name) RULE(name, KILL)
static const struct syscall_rule syscall_rules[] = {
    KILL(ptrace),
    RULE(prctl, ERRNO(EPERM)), /* TODO: PR_SET_NAME or PR_GET_NAME should be allowed */
    RULE(syslog, ERRNO(EPERM)),
    RULE(socket, ERRNO(EPERM)),
    ALLOW(execve),
    ALLOW(clone),
    ALLOW(vfork),
    ALLOW(wait4),
    ALLOW(dup),
    ALLOW(dup2),
    ALLOW(capget),
    ALLOW(kill),
    ALLOW(tgkill),

    // safe
    ALLOW(futex),
    ALLOW(exit),
    ALLOW(pipe),
    ALLOW(pipe2),
    ALLOW(brk),
    ALLOW(mmap),
    ALLOW(mprotect),
    ALLOW(munmap),
    ALLOW(mremap),
    ALLOW(madvise),
    ALLOW(rt_sigaction),
    ALLOW(rt_sigprocmask),
    ALLOW(rt_sigreturn),
    ALLOW(nanosleep),
    ALLOW(getrlimit),
    ALLOW(poll),
    ALLOW(exit_group),
    ALLOW(getpid),
    ALLOW(getuid),
    ALLOW(getgid),
    ALLOW(geteuid),
    ALLOW(getegid),
    ALLOW(getresuid),
    ALLOW(getresgid),
    ALLOW(gettimeofday),
    ALLOW(clock_gettime),
    ALLOW(set_tid_address),
    ALLOW(getdents),
    ALLOW(arch_prctl),
    ALLOW(set_robust_list),
    ALLOW(get_robust_list),
    ALLOW(sigaltstack),
    ALLOW(uname), // dummy? /proc/sys/kernel/*?
    ALLOW(getcwd),
    ALLOW(getppid),
    ALLOW(getpgrp),
    ALLOW(getrandom),
    ALLOW(ppoll),

    // ????
    ALLOW(utimensat),
    ALLOW(futimesat),
    ALLOW(getxattr),
    ALLOW(getgroups),
    ALLOW(newfstatat),

    // ???
    ALLOW(fadvise64),
    ALLOW(readlink),
    ALLOW(open),
    ALLOW(openat),
    ALLOW(stat),
    ALLOW(close),
    ALLOW(read),
    ALLOW(readv),
    ALLOW(pread64),
    ALLOW(write),
    ALLOW(writev),
    ALLOW(pwrite64),
    ALLOW(lstat),
    ALLOW(fstat),
    ALLOW(fcntl),
    ALLOW(ioctl),
    ALLOW(lseek),
    ALLOW(access),
    ALLOW(faccessat),
    ALLOW(unlinkat),
    ALLOW(fchdir),
    ALLOW(getpeername),
    ALLOW(getrusage),
    ALLOW(select),
    ALLOW(sched_getaffinity),
};
static const int syscall_rules_count = sizeof(syscall_rules) / sizeof(struct syscall_rule);
#undef KILL
#undef ALLOW
#undef RULE

#endif
