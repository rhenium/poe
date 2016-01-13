#include "sandbox.h"

struct syscall_rule {
    int syscall;
    uint32_t action;
};
#define RULE(name, action) { SCMP_SYS(name), SCMP_ACT_##action }
static const struct syscall_rule syscall_rules[] = {
    RULE(ptrace,        ERRNO(EPERM)),
    RULE(prctl,         ERRNO(EPERM)),
    RULE(execve,        ALLOW),
    RULE(clone,         ALLOW),
    RULE(vfork,         ALLOW),
    RULE(wait4,         ALLOW),
    RULE(dup,           ALLOW),
    RULE(dup2,          ALLOW),
    RULE(capget,        ALLOW),
    RULE(kill,          ALLOW),
    RULE(tgkill,        ALLOW),

    // safe
    RULE(futex,         ALLOW),
    RULE(exit,          ALLOW),
    RULE(pipe,          ALLOW),
    RULE(pipe2,         ALLOW),
    RULE(brk,           ALLOW),
    RULE(mmap,          ALLOW),
    RULE(mprotect,      ALLOW),
    RULE(munmap,        ALLOW),
    RULE(mremap,        ALLOW),
    RULE(madvise,       ALLOW),
    RULE(rt_sigaction,  ALLOW),
    RULE(rt_sigprocmask,ALLOW),
    RULE(rt_sigreturn,  ALLOW),
    RULE(nanosleep,     ALLOW),
    RULE(getrlimit,     ALLOW),
    RULE(poll,          ALLOW),
    RULE(exit_group,    ALLOW),
    RULE(getpid,        ALLOW),
    RULE(getuid,        ALLOW),
    RULE(getgid,        ALLOW),
    RULE(geteuid,       ALLOW),
    RULE(getegid,       ALLOW),
    RULE(getresuid,     ALLOW),
    RULE(getresgid,     ALLOW),
    RULE(gettimeofday,  ALLOW),
    RULE(clock_gettime, ALLOW),
    RULE(set_tid_address, ALLOW),
    RULE(getdents,      ALLOW),
    RULE(arch_prctl,    ALLOW),
    RULE(set_robust_list, ALLOW),
    RULE(get_robust_list, ALLOW),
    RULE(sigaltstack,   ALLOW),
    RULE(uname,         ALLOW), // dummy? /proc/sys/kernel/*?
    RULE(getcwd,        ALLOW),
    RULE(getppid,       ALLOW),
    RULE(getpgrp,       ALLOW),
    RULE(getrandom,     ALLOW),
    RULE(ppoll,         ALLOW),

    // ????
    RULE(socket,        ERRNO(ENOSYS)),
    RULE(utimensat,     ALLOW),
    RULE(futimesat,     ALLOW),
    RULE(getxattr,      ALLOW),
    RULE(getgroups,     ALLOW),
    RULE(newfstatat,    ALLOW),

    // ???
    RULE(fadvise64,     ALLOW),
    RULE(readlink,      ALLOW),
    RULE(open,          ALLOW),
    RULE(openat,        ALLOW),
    RULE(stat,          ALLOW),
    RULE(close,         ALLOW),
    RULE(read,          ALLOW),
    RULE(readv,         ALLOW),
    RULE(pread64,       ALLOW),
    RULE(write,         ALLOW),
    RULE(writev,        ALLOW),
    RULE(pwrite64,      ALLOW),
    RULE(lstat,         ALLOW),
    RULE(fstat,         ALLOW),
    RULE(fcntl,         ALLOW),
    RULE(ioctl,         ALLOW),
    RULE(lseek,         ALLOW),
    RULE(access,        ALLOW),
    RULE(faccessat,     ALLOW),
    RULE(unlinkat,      ALLOW),
    RULE(fchdir,        ALLOW),
    RULE(getpeername,   ALLOW),
    RULE(syslog,        ERRNO(EPERM)),
    RULE(getrusage,     ALLOW),
};
#undef RULE
static const int syscall_rules_count = sizeof(syscall_rules) / sizeof(struct syscall_rule);

void
poe_init_seccomp(uint32_t act)
{
    scmp_filter_ctx ctx = seccomp_init(act);
    if (!ctx) ERROR("seccomp_init() failed");

    for (int i = 0; i < syscall_rules_count; i++) {
        struct syscall_rule rule = syscall_rules[i];
        if (seccomp_rule_add(ctx, rule.action, rule.syscall, 0) < 0) ERROR("seccomp_rule_add() failed");
    }

    int rc = seccomp_load(ctx);
    if (rc < 0) ERROR("seccomp_load() failed: %s", strerror(-rc));

    seccomp_release(ctx);
}
