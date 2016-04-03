#include "sandbox.h"

static noreturn void finish(enum poe_exit_reason reason, int status, const char *fmt, ...)
{
	int xx[] = { reason, status };
	fwrite(xx, sizeof(int), 2, stderr);
	if (fmt) {
		va_list args;
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
	exit(0);
}

static void handle_stdout(int fd, int orig_fd)
{
	assert(PIPE_BUF % 4 == 0);
	uint32_t buf[PIPE_BUF / 4 + 2];
	ssize_t n = read(fd, (char *)(buf + 2), PIPE_BUF);
	if (n < 0)
		bug("read from stdout/err pipe");
	buf[0] = (uint32_t)orig_fd;
	buf[1] = (uint32_t)n;
	if (write(STDOUT_FILENO, buf, n + 8) < 0)
		bug("write to stdout failed");
}

static void handle_signal(pid_t mpid, struct signalfd_siginfo *si)
{
	if (si->ssi_signo == SIGINT || si->ssi_signo == SIGTERM ||
			si->ssi_signo == SIGHUP)
		finish(POE_TIMEDOUT, -1, "Supervisor terminated");
	if (si->ssi_signo != SIGCHLD)
		bug("unknown signal %d", si->ssi_signo);

	int status;
	pid_t spid;
	while ((spid = waitpid(-mpid, &status, WNOHANG | __WALL)) > 0) {
		if (spid == mpid && WIFEXITED(status)) {
			finish(POE_SUCCESS, WEXITSTATUS(status), NULL);
		} else if (spid == mpid && WIFSIGNALED(status)) {
			int sig = WTERMSIG(status);
			finish(POE_SIGNALED, -1, "Program terminated with signal %d (%s)", sig, strsignal(sig));
		} else if (WIFSTOPPED(status)) {
			switch (status >> 16 & 0xff) {
			case PTRACE_EVENT_SECCOMP:
				errno = 0;
				int syscalln = ptrace(PTRACE_PEEKUSER, spid, sizeof(long) * ORIG_RAX);
				if (errno)
					bug("ptrace(PTRACE_PEEKUSER) failed");
				char *name = poe_seccomp_syscall_resolve(syscalln);
				finish(POE_SIGNALED, -1, "System call %s is blocked", name);
				break;
			case PTRACE_EVENT_CLONE:
			case PTRACE_EVENT_FORK:
			case PTRACE_EVENT_VFORK:
				ptrace(PTRACE_CONT, spid, 0, 0);
				break;
			default:
				ptrace(PTRACE_CONT, spid, 0, WSTOPSIG(status));
				break;
			}
		}
	}
	if (spid < 0) {
		if (errno == ECHILD)
			bug("child dies too early (before raising SIGSTOP)");
		else
			bug("waitpid failed");
	}
}

int main(int argc, char *argv[])
{
	if (argc < 5)
		die("usage: runner basedir overlaydir sourcefile cmdl..");

	struct playground *pg = poe_playground_init(argv[1], argv[2]);
	if (!pg)
		die("playground init failed");
	if (poe_playground_init_command_line(pg, argv + 4, argv[3]))
		die("copy program failed");

	int stdout_fd[2], stderr_fd[2], child_fd[2];
	if (pipe2(stdout_fd, O_DIRECT))
		bug("pipe2 failed");
	if (pipe2(stderr_fd, O_DIRECT))
		bug("pipe2 failed");
	if (pipe2(child_fd, O_DIRECT | O_CLOEXEC))
		bug("pipe2 failed");

	// init cgroup: create root hierarchy and setup controllers
	if (poe_cgroup_init())
		die("failed to init cgroup");

	// TODO: CLONE_NEWUSER
	pid_t pid = (pid_t)syscall(SYS_clone, SIGCHLD | CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNET, 0);
	if (pid < 0)
		bug("clone failed");
	if (!pid) {
		poe_child_do(pg, stdout_fd, stderr_fd, child_fd);
		bug("unreachable");
	}

	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0)
		bug("epoll_create1 failed");

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGHUP);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	int signal_fd = signalfd(-1, &mask, 0);
	if (signal_fd < 0)
		bug("signalfd failed");

	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer_fd < 0)
		bug("timerfd_create failed");
	if (timerfd_settime(timer_fd, 0, &(struct itimerspec) { .it_value.tv_sec = POE_TIME_LIMIT }, NULL))
		bug("timerfd_settime failed");

#define ADD(_fd__) do if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd__, &(struct epoll_event) { .data.fd = _fd__, .events = EPOLLIN })) \
	bug("EPOLL_CTL_ADD failed"); while (0)
	ADD(signal_fd);
	ADD(timer_fd);
	ADD(child_fd[0]);
	ADD(stdout_fd[0]);
	ADD(stderr_fd[0]);

	if (ptrace(PTRACE_SEIZE, pid, NULL,
				PTRACE_O_TRACECLONE |
				PTRACE_O_TRACEFORK |
				PTRACE_O_TRACESECCOMP |
				PTRACE_O_TRACEVFORK))
		bug("ptrace failed");

	if (poe_cgroup_add(pid))
		die("failed cgroup add");

	while (true) {
		struct epoll_event events[10];
		int n = epoll_wait(epoll_fd, events, sizeof(events) / sizeof(events[0]), -1);
		if (n < 0)
			bug("epoll_wait failed");

		for (int i = 0; i < n; i++) {
			struct epoll_event *ev = &events[i];
			if (ev->events & EPOLLERR) {
				// fd closed
				close(ev->data.fd);
			}
			if (ev->events & EPOLLIN) {
				if (ev->data.fd == stdout_fd[0]) {
					handle_stdout(ev->data.fd, STDOUT_FILENO);
				} else if (ev->data.fd == stderr_fd[0]) {
					handle_stdout(ev->data.fd, STDERR_FILENO);
				} else if (ev->data.fd == signal_fd) {
					struct signalfd_siginfo si;
					if (sizeof(si) != read(signal_fd, &si, sizeof(si)))
						die("partial read signalfd");
					handle_signal(pid, &si);
				} else if (ev->data.fd == timer_fd) {
					finish(POE_TIMEDOUT, -1, NULL);
				} else if (ev->data.fd == child_fd[0]) {
					char buf[PIPE_BUF];
					ssize_t nx = read(child_fd[0], buf, sizeof(buf));
					if (nx > 0) // TODO
						die("child err: %s", strndupa(buf, nx));
				}
			}
		}
	}

	bug("unreachable");
}
