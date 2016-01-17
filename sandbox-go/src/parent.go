package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"os/signal"
	"runtime"
	"syscall"
	"time"
)

type childOutput struct {
	fd  int
	n   int
	buf []byte
}

func doParent(mpid int, stdin_fd, stdout_fd, stderr_fd [2]int) resultPack {
	syscall.Close(stdin_fd[0])
	syscall.Close(stdout_fd[1])
	syscall.Close(stderr_fd[1])

	exit_chan := make(chan resultPack)

	if err := SetupScope(uint32(mpid)); err != nil {
		poePanic(err, "setup systemd scope unit failed")
	}

	go func() {
		runtime.LockOSThread()

		sig_chan := make(chan os.Signal, 1)
		signal.Notify(sig_chan, syscall.SIGCHLD)
		signal.Notify(sig_chan, syscall.SIGTERM)
		signal.Notify(sig_chan, syscall.SIGINT)

		if err := PtraceSeize(mpid, syscall.PTRACE_O_TRACECLONE|syscall.PTRACE_O_TRACEFORK|PTRACE_O_TRACESECCOMP|syscall.PTRACE_O_TRACEVFORK); err != nil {
			poePanic(error(err), "PTRACE_SEIZE failed")
		}

		for {
			sig := <-sig_chan
			switch sig {
			case syscall.SIGCHLD:
				if res := handleSigchld(mpid); res != nil {
					exit_chan <- *res
				}
			case syscall.SIGTERM, syscall.SIGINT:
				exit_chan <- resultPack{POE_TIMEDOUT, -1, "Supervisor terminated"}
			}
		}
	}()

	// TODO: output order is not preserved
	out_chan := make(chan childOutput, 1)
	go func() {
		epfd, err := syscall.EpollCreate1(0)
		if err != nil {
			poePanic(err, "epoll_create1 failed")
		}
		defer syscall.Close(epfd)

		event1 := syscall.EpollEvent{Events: syscall.EPOLLIN, Fd: int32(stdout_fd[0]), Pad: 1} // Fd/Pad is epoll_data_t (userdata)
		if err := syscall.EpollCtl(epfd, syscall.EPOLL_CTL_ADD, stdout_fd[0], &event1); err != nil {
			poePanic(err, "epoll_ctl (EPOLL_CTL_ADD) failed")
		}
		event2 := syscall.EpollEvent{Events: syscall.EPOLLIN, Fd: int32(stderr_fd[0]), Pad: 2}
		if err := syscall.EpollCtl(epfd, syscall.EPOLL_CTL_ADD, stderr_fd[0], &event2); err != nil {
			poePanic(err, "epoll_ctl (EPOLL_CTL_ADD) failed")
		}

		var events [32]syscall.EpollEvent
		for {
			en, err := syscall.EpollWait(epfd, events[:], -1)
			if err != nil {
				poePanic(err, "epoll_wait failed")
			}

			var buf [1024]byte // TODO: pipe size?
			for ev := 0; ev < en; ev++ {
				for {
					n, err := syscall.Read(int(events[ev].Fd), buf[:])
					if err != nil {
						break
					}
					if n > 0 {
						nbuf := make([]byte, n)
						copy(nbuf, buf[:n])
						out_chan <- childOutput{int(events[ev].Pad), n, nbuf}
					}
				}
			}
		}
	}()

	go func() {
		for {
			out := <-out_chan
			var outbuf bytes.Buffer
			binary.Write(&outbuf, binary.LittleEndian, int32(out.fd))
			binary.Write(&outbuf, binary.LittleEndian, int32(out.n))
			if _, err := os.Stdout.Write(outbuf.Bytes()); err != nil {
				poePanic(err, "stdout write failed")
			}
			if _, err := os.Stdout.Write(out.buf); err != nil {
				poePanic(err, "stdout write failed")
			}
		}
	}()

	go func() {
		<-time.After(3 * time.Second)
		exit_chan <- resultPack{POE_TIMEDOUT, -1, ""}
	}()

	if _, err := syscall.Write(stdin_fd[1], []byte{0}); err != nil {
		poePanic(err, "write to stdin failed")
	}

	return <-exit_chan
}

func handleSigchld(mpid int) *resultPack {
	for {
		var status syscall.WaitStatus
		var spid int
		var err error
		spid, err = syscall.Wait4(-mpid, &status, syscall.WNOHANG|syscall.WALL, nil)
		if err != nil {
			poePanic(err, "wait4 failed")
		} else if spid == 0 {
			return nil
		}

		if spid == mpid && status.Exited() {
			return &resultPack{POE_SUCCESS, status.ExitStatus(), ""}
		} else if spid == mpid && status.Signaled() {
			return &resultPack{POE_SIGNALED, -1, fmt.Sprintf("Program terminated with signal %d (%s)", int(status.Signal()), status.Signal().String())}
		} else if status.Stopped() {
			e := status >> 16 & 0xff
			switch e {
			case PTRACE_EVENT_SECCOMP:
				if res := handleSyscall(spid); res != nil {
					return res
				}
			case syscall.PTRACE_EVENT_CLONE, syscall.PTRACE_EVENT_FORK, syscall.PTRACE_EVENT_VFORK:
				syscall.PtraceCont(spid, 0)
			default:
				syscall.PtraceCont(spid, int(status.StopSignal()))
			}
		}
	}
}

func handleSyscall(spid int) *resultPack {
	var regs syscall.PtraceRegs
	if err := syscall.PtraceGetRegs(spid, &regs); err != nil {
		poePanic(err, "getreg failed")
	}

	switch regs.Orig_rax {
	case syscall.SYS_KILL:
		goto allow
	default:
		goto kill
	}

kill:
	if err := syscall.Kill(spid, syscall.SIGKILL); err != nil {
		poePanic(err, "failed to kill process which called prohibited syscall")
	} else if name, err := ResolveSeccompSyscallName(regs.Orig_rax); err != nil {
		poePanic(err, "seccomp_syscall_resolve_num_arch() failed")
	} else {
		return &resultPack{POE_SIGNALED, -1, fmt.Sprintf("System call %s is blocked", name)}
	}

allow:
	if err := syscall.PtraceCont(spid, 0); err != nil {
		poePanic(err, "PTRACE_CONT failed")
	}
	return nil
}
