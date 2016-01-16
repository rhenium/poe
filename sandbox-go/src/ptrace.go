package main

import (
	"syscall"
)

const (
	PTRACE_EVENT_SECCOMP  = 0x7 // missing in syscall (!!)
	PTRACE_O_TRACESECCOMP = (1 << PTRACE_EVENT_SECCOMP)
	PTRACE_SEIZE          = 0x4206
)

func PtraceSeize(pid int, flags uintptr) error {
	_, _, err := syscall.Syscall6(syscall.SYS_PTRACE, PTRACE_SEIZE, uintptr(pid), 0, flags, 0, 0)
	if err != 0 {
		return error(err)
	} else {
		return nil
	}
}
