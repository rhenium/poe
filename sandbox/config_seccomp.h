#ifndef CONFIG_SECCOMP_H
#define CONFIG_SECCOMP_H

#include <seccomp.h>

struct syscall_rule {
	int syscall;
	uint32_t action;
};

#define RULE(name, action) { SCMP_SYS(name), SCMP_ACT_##action }
#define ALLOW(name) RULE(name, ALLOW)
#define KILL(name) RULE(name, TRACE(2))
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
	ALLOW(pause),
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
