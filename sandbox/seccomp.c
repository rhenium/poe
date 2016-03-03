#include "sandbox.h"

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
