package main

import (
	"fmt"
	"github.com/seccomp/libseccomp-golang"
	"syscall"
)

func Seccomp(trap, arg1, arg2, arg3 uintptr) (r1 uintptr) {
	r1, _, err := syscall.RawSyscall(trap, arg1, arg2, arg3)
	if err != 0 {
		name, err2 := seccomp.ScmpSyscall(trap).GetNameByArch(seccomp.ArchNative)
		if err2 != nil {
			panic(fmt.Sprintf("syscall %d failed (%s)", int(trap), err.Error()))
		} else {
			panic(fmt.Sprintf("syscall %s failed (%s)", name, err.Error()))
		}
	}
	return r1
}

func SetupSeccomp() error {
	filter, err := seccomp.NewFilter(seccomp.ActTrace.SetReturnCode(0))
	if err != nil {
		return err
	}

	for _, rule := range seccompRules() {
		syscall, err := seccomp.GetSyscallFromName(rule.SyscallName)
		if err != nil {
			return err
		}
		filter.AddRule(syscall, rule.Action)
	}

	return filter.Load()
}

func ResolveSeccompSyscallName(syscall uint64) (string, error) {
	return seccomp.ScmpSyscall(syscall).GetNameByArch(seccomp.ArchNative)
}

type SeccompRule struct {
	SyscallName string
	Action      seccomp.ScmpAction
}

func seccompRules() []SeccompRule {
	return []SeccompRule{
		{"ptrace", seccomp.ActErrno.SetReturnCode(int16(syscall.EPERM))},
		{"prctl", seccomp.ActErrno.SetReturnCode(int16(syscall.EPERM))},
		{"execve", seccomp.ActAllow},
		{"clone", seccomp.ActAllow},
		{"vfork", seccomp.ActAllow},
		{"wait4", seccomp.ActAllow},
		{"dup", seccomp.ActAllow},
		{"dup2", seccomp.ActAllow},
		{"capget", seccomp.ActAllow},
		{"kill", seccomp.ActAllow},
		{"tgkill", seccomp.ActAllow},

		// safe
		{"futex", seccomp.ActAllow},
		{"exit", seccomp.ActAllow},
		{"pipe", seccomp.ActAllow},
		{"pipe2", seccomp.ActAllow},
		{"brk", seccomp.ActAllow},
		{"mmap", seccomp.ActAllow},
		{"mprotect", seccomp.ActAllow},
		{"munmap", seccomp.ActAllow},
		{"mremap", seccomp.ActAllow},
		{"madvise", seccomp.ActAllow},
		{"rt_sigaction", seccomp.ActAllow},
		{"rt_sigprocmask", seccomp.ActAllow},
		{"rt_sigreturn", seccomp.ActAllow},
		{"nanosleep", seccomp.ActAllow},
		{"getrlimit", seccomp.ActAllow},
		{"poll", seccomp.ActAllow},
		{"exit_group", seccomp.ActAllow},
		{"getpid", seccomp.ActAllow},
		{"getuid", seccomp.ActAllow},
		{"getgid", seccomp.ActAllow},
		{"geteuid", seccomp.ActAllow},
		{"getegid", seccomp.ActAllow},
		{"getresuid", seccomp.ActAllow},
		{"getresgid", seccomp.ActAllow},
		{"gettimeofday", seccomp.ActAllow},
		{"clock_gettime", seccomp.ActAllow},
		{"set_tid_address", seccomp.ActAllow},
		{"getdents", seccomp.ActAllow},
		{"arch_prctl", seccomp.ActAllow},
		{"set_robust_list", seccomp.ActAllow},
		{"get_robust_list", seccomp.ActAllow},
		{"sigaltstack", seccomp.ActAllow},
		{"uname", seccomp.ActAllow}, // dummy? /proc/sys/kernel/*?
		{"getcwd", seccomp.ActAllow},
		{"getppid", seccomp.ActAllow},
		{"getpgrp", seccomp.ActAllow},
		{"getrandom", seccomp.ActAllow},
		{"ppoll", seccomp.ActAllow},

		// ??
		{"socket", seccomp.ActErrno.SetReturnCode(int16(syscall.EPERM))},
		{"utimensat", seccomp.ActAllow},
		{"futimesat", seccomp.ActAllow},
		{"getxattr", seccomp.ActAllow},
		{"getgroups", seccomp.ActAllow},
		{"newfstatat", seccomp.ActAllow},

		// ?
		{"fadvise64", seccomp.ActAllow},
		{"readlink", seccomp.ActAllow},
		{"open", seccomp.ActAllow},
		{"openat", seccomp.ActAllow},
		{"stat", seccomp.ActAllow},
		{"close", seccomp.ActAllow},
		{"read", seccomp.ActAllow},
		{"readv", seccomp.ActAllow},
		{"pread64", seccomp.ActAllow},
		{"write", seccomp.ActAllow},
		{"writev", seccomp.ActAllow},
		{"pwrite64", seccomp.ActAllow},
		{"lstat", seccomp.ActAllow},
		{"fstat", seccomp.ActAllow},
		{"fcntl", seccomp.ActAllow},
		{"ioctl", seccomp.ActAllow},
		{"lseek", seccomp.ActAllow},
		{"access", seccomp.ActAllow},
		{"faccessat", seccomp.ActAllow},
		{"unlinkat", seccomp.ActAllow},
		{"fchdir", seccomp.ActAllow},
		{"getpeername", seccomp.ActAllow},
		{"syslog", seccomp.ActErrno.SetReturnCode(int16(syscall.EPERM))},
		{"getrusage", seccomp.ActAllow},
		{"select", seccomp.ActAllow},
		{"sched_getaffinity", seccomp.ActAllow},
	}
}
