package main

import (
	"errors"
	"fmt"
	"os"
	"os/user"
	"strconv"
	"syscall"
)

const (
	PR_SET_NO_NEW_PRIVS = 38
	username            = "nobody"
)

type mountPoint struct {
	source string
	target string
	fstype string
	flags  uintptr
	data   string
}

func doChild(rootdir string, progfile string, plan plan, stdin_fd, stdout_fd, stderr_fd [2]int) (error, string) {
	if os.Getpid() != 1 {
		return errors.New("not cloned?"), ""
	}

	if err := syscall.Dup2(stdin_fd[0], 0); err != nil {
		return err, "dup2 failed"
	}
	syscall.Close(stdin_fd[0])
	syscall.Close(stdin_fd[1])
	if err := syscall.Dup2(stdout_fd[1], 1); err != nil {
		return err, "dup2 failed"
	}
	syscall.Close(stdout_fd[0])
	syscall.Close(stdout_fd[1])
	if err := syscall.Dup2(stderr_fd[1], 2); err != nil {
		return err, "dup2 failed"
	}
	syscall.Close(stderr_fd[0])
	syscall.Close(stderr_fd[1])

	if _, _, err := syscall.Syscall(syscall.SYS_PRCTL, syscall.PR_SET_PDEATHSIG, uintptr(syscall.SIGKILL), 0); err != 0 {
		return error(err), "PR_SET_PDEATHSIG failed"
	}

	if err := syscall.Sethostname([]byte("poe-sandbox")); err != nil {
		return err, "sethostname failed"
	}

	mounts := []mountPoint{
		{"none", "/", "", syscall.MS_PRIVATE | syscall.MS_REC, ""},
		{rootdir, rootdir, "bind", syscall.MS_BIND | syscall.MS_REC, ""},
		// { "none",   rootdir + "/proc", "proc",         syscall.MS_NOSUID | syscall.MS_NOEXEC | syscall.MS_NODEV, "" },
		// { "none",   rootdir + "/dev",  "devtmpfs",     syscall.MS_NOSUID | syscall.MS_NOEXEC, "" },
		// { "none",   rootdir + "/dev/shm", "tmpfs",        syscall.MS_NOSUID | syscall.MS_NODEV, "" },
	}
	for _, point := range mounts {
		if err := syscall.Mount(point.source, point.target, point.fstype, point.flags, point.data); err != nil {
			return err, fmt.Sprintf("mount '%s' on '%s' (%s) failed", point.source, point.target, point.fstype)
		}
	}

	if err := syscall.Chroot(rootdir); err != nil {
		return err, "chroot failed"
	}

	if _, err := syscall.Setsid(); err != nil {
		return err, "setsid failed"
	}

	pw, err := user.Lookup(username)
	if err != nil {
		return err, "getpwnam failed"
	}
	uid, err := strconv.Atoi(pw.Uid)
	if err != nil {
		return err, "atoi error"
	}
	gid, err := strconv.Atoi(pw.Gid)
	if err != nil {
		return err, "atoi error"
	}
	// TODO: initgroups

	if err := syscall.Setresgid(gid, gid, gid); err != nil {
		return err, "setresgid failed"
	}

	if err := syscall.Setresuid(uid, uid, uid); err != nil {
		return err, "setresuid failed"
	}

	if err := syscall.Chdir("/tmp"); err != nil {
		return err, "chdir failed"
	}

	// stop
	os.Stdin.Read(make([]byte, 1))
	// be traced
	if _, _, err := syscall.Syscall(syscall.SYS_PRCTL, PR_SET_NO_NEW_PRIVS, 1, 0); err != 0 {
		return error(err), "PR_SET_NO_NEW_PRIVS failed"
	}

	if err := SetupSeccomp(); err != nil {
		return err, "seccomp fail"
	}

	envp := []string{
		"PATH=/opt/bin:/usr/bin",
		"USER=" + username,
		"LOGNAME=" + username,
	}

	cmdl := make([]string, 0, len(plan.Compiler.Command) + len(plan.Extra) - 1)
	for _, arg := range plan.Compiler.Command {
		if arg == "PROGRAM" {
			cmdl = append(cmdl, progfile)
		} else if arg == "EXTRA" {
			cmdl = append(cmdl, plan.Extra...)
		} else {
			cmdl = append(cmdl, arg)
		}
	}

	if err := syscall.Exec(cmdl[0], cmdl, envp); err != nil {
		return err, "execve failed"
	}

	return errors.New("unreachable"), ""
}
