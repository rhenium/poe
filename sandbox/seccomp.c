#include "sandbox.h"
#include "config_seccomp.h"
#include <seccomp.h>

warn_unused int poe_seccomp_init(void)
{
	assert(syscall(SYS_getpid) == 1);

	int ret = 0;
	scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_TRACE(0));
	if (!ctx)
		return error("seccomp_init() failed");

	for (int i = 0; i < syscall_rules_count; i++) {
		struct syscall_rule rule = syscall_rules[i];
		if (seccomp_rule_add(ctx, rule.action, rule.syscall, 0)) {
			ret = error("seccomp_rule_add() failed");
			goto release;
		}
	}

	if ((ret = seccomp_load(ctx)))
		error("seccomp_load() failed: %s", strerror(-ret));

release:
	seccomp_release(ctx);
	return ret;
}

/* Return value must be freed */
char *poe_seccomp_syscall_resolve(int n)
{
	char *rule = seccomp_syscall_resolve_num_arch(SCMP_ARCH_NATIVE, n);
	if (!rule)
		bug("seccomp_syscall_resolve_num_arch() failed");

	return rule;
}
