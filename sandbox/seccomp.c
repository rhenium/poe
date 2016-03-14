#include "sandbox.h"
#include <seccomp.h>
#include <sys/ptrace.h>

void
poe_seccomp_init(void)
{
    assert(syscall(SYS_getpid) == 1);
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_TRACE(0)); /* for a workaround, we can't kill TRACEd process, so default to KILL */
    if (!ctx) ERROR("seccomp_init() failed");

    for (int i = 0; i < syscall_rules_count; i++) {
        struct syscall_rule rule = syscall_rules[i];
        if (seccomp_rule_add(ctx, rule.action, rule.syscall, 0) < 0) ERROR("seccomp_rule_add() failed");
    }

    int rc = seccomp_load(ctx);
    if (rc < 0) ERROR("seccomp_load() failed: %s", strerror(-rc));

    seccomp_release(ctx);
}

char *
poe_seccomp_handle_syscall(pid_t pid, unsigned long edata)
{
    assert(syscall(SYS_getpid) != 1);
    errno = 0;
    int syscalln = ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * ORIG_RAX);
    if (errno) ERROR("ptrace(PTRACE_PEEKUSER) failed");

    if (edata == 2) goto kill; // 2 is KILL sign
    switch (syscalln) {
    default:
        goto kill;
    }
kill:
    kill(pid, SIGKILL);
    char *rule = seccomp_syscall_resolve_num_arch(SCMP_ARCH_NATIVE, syscalln);
    if (!rule) ERROR("seccomp_syscall_resolve_num_arch() failed");
    return rule;
/*allowed:
    ptrace(PTRACE_CONT, pid, 0, 0);*/
}
