package main

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"os"
	"runtime"
	"syscall"
)

type plan struct {
	Source      string
	Command     []string
	Base        string
	Environment string
}

type poeExitReason int

const (
	POE_SUCCESS  = 0
	POE_SIGNALED = 1
	POE_TIMEDOUT = 2
)

type resultPack struct {
	result poeExitReason
	status int
	msg    string
}

func poePanic(err error, msg string) {
	cleanup()
	panic(fmt.Sprintf("%s (%s)", msg, err.Error()))
}

func cleanup() {
	err1 := StopScope()
	err2 := PlaygroundDestroy()

	if err1 != nil {
		panic(err1)
	}
	if err2 != nil {
		panic(err2)
	}
}

func main() {
	runtime.GOMAXPROCS(3)
	dec := json.NewDecoder(os.Stdin)
	var plan plan
	if err := dec.Decode(&plan); err != nil {
		panic(fmt.Sprintf("failed to parse plan: %s", err))
	}
	//fmt.Fprintf(os.Stderr, "plan: %+v\n", plan)

	runtime.LockOSThread()
	if err := syscall.Setresuid(0, 0, 0); err != nil {
		poePanic(err, "setuid failed")
	}
	if err := InitializeSystemdBus(); err != nil {
		poePanic(err, "failed to connect to systemd")
	}
	runtime.UnlockOSThread()

	rootdir, errx := PlaygroundCreate(plan.Base, plan.Environment)
	if errx != nil {
		poePanic(errx, "playground_create failed")
	}

	progfile, errx := PlaygroundCopy(rootdir, plan.Source)
	if errx != nil {
		poePanic(errx, "playground_copy failed")
	}

	var stdin_fd, stdout_fd, stderr_fd [2]int
	if err := syscall.Pipe2(stdin_fd[:], 0); err != nil {
		poePanic(err, "pipe2 failed")
	}
	if err := syscall.Pipe2(stdout_fd[:], syscall.O_DIRECT); err != nil {
		poePanic(err, "pipe2 failed")
	}
	if err := syscall.Pipe2(stderr_fd[:], syscall.O_DIRECT); err != nil {
		poePanic(err, "pipe2 failed")
	}

	pid_, _, err := syscall.Syscall(syscall.SYS_CLONE, uintptr(syscall.SIGCHLD|syscall.CLONE_NEWIPC|syscall.CLONE_NEWNS|syscall.CLONE_NEWPID|syscall.CLONE_NEWUTS|syscall.CLONE_NEWNET), 0, 0)
	pid := int(pid_)
	if err != 0 {
		poePanic(error(err), "clone failed")
	} else if pid == 0 {
		runtime.LockOSThread()
		if err, msg := doChild(rootdir, progfile, plan, stdin_fd, stdout_fd, stderr_fd); err != nil {
			fmt.Fprintf(os.Stderr, "%s (%s)", msg, err.Error())
			os.Exit(127)
		}
		// unreachable
	} else {
		res := doParent(pid, stdin_fd, stdout_fd, stderr_fd)
		cleanup()
		var buf bytes.Buffer
		binary.Write(&buf, binary.LittleEndian, int32(res.result))
		binary.Write(&buf, binary.LittleEndian, int32(res.status))
		if _, err := os.Stderr.Write(buf.Bytes()); err != nil {
			poePanic(err, "stderr write failed")
		}
		if _, err := os.Stderr.Write([]byte(res.msg)); err != nil {
			poePanic(err, "stderr write failed")
		}
		os.Exit(0)
		// unreachable
	}
}
