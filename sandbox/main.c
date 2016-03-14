#include "sandbox.h"
#include <sys/ptrace.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

static void
finish_exit(enum poe_exit_reason reason, int status, const char *fmt, ...)
{
    assert(syscall(SYS_getpid) != 1);
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

static void
handle_signal(pid_t mpid, struct signalfd_siginfo *si)
{
    switch (si->ssi_signo) {
    case SIGCHLD:
        while (true) {
            int status;
            pid_t spid = waitpid(-mpid, &status, WNOHANG|__WALL);
            C_SYSCALL(spid);
            if (!spid) break;

            if (WIFEXITED(status) && spid == mpid) {
                finish_exit(POE_SUCCESS, WEXITSTATUS(status), NULL);
            } else if (WIFSIGNALED(status) && spid == mpid) {
                finish_exit(POE_SIGNALED, -1, "Program terminated with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
            } else if (WIFSTOPPED(status)) {
                unsigned long edata;
                int e = status >> 16 & 0xff;
                switch (e) {
                case PTRACE_EVENT_SECCOMP:
                    C_SYSCALL(ptrace(PTRACE_GETEVENTMSG, spid, 0, &edata));
                    char *str = poe_seccomp_handle_syscall(spid, edata);
                    if (str) {
                        finish_exit(POE_SIGNALED, -1, "System call %s is blocked", str);
                        free(str);
                    }
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
        break;
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
        finish_exit(POE_TIMEDOUT, -1, "Supervisor terminated");
    }
}

static void
handle_timer(pid_t pid)
{
    (void)pid;
    finish_exit(POE_TIMEDOUT, -1, NULL);
}

static void
handle_stdout(int fd, int orig_fd)
{
    char buf[PIPE_BUF];

    ssize_t n = read(fd, buf, sizeof(buf));
    C_SYSCALL(n);
    uint32_t len = (uint32_t)n;
    C_SYSCALL(write(STDOUT_FILENO, &orig_fd, sizeof(orig_fd)));
    C_SYSCALL(write(STDOUT_FILENO, &len, sizeof(len)));
    C_SYSCALL(write(STDOUT_FILENO, buf, len));
}

static char **
construct_cmdl(int cmdl, char *cmd[], char *prog)
{
    for (int i = 0; i < cmdl; i++) {
        if (!strcmp(cmd[i], "{}")) {
            cmd[i] = prog;
        }
    }

    return cmd;
}

int
main(int argc, char *argv[])
{
    if (argc < 5) ERROR("usage: runner basedir overlaydir sourcefile cmdl..\n");

    char *root = poe_init_playground(argv[1], argv[2]);
    char *prog = poe_copy_program_to_playground(root, argv[3]);
    char **cmdl = construct_cmdl(argc - 4, argv + 4, prog);

    int stdout_fd[2], stderr_fd[2], errfd[2];
    C_SYSCALL(pipe2(stdout_fd, O_DIRECT));
    C_SYSCALL(pipe2(stderr_fd, O_DIRECT));
    C_SYSCALL(pipe2(errfd, O_DIRECT|O_CLOEXEC));

    // init cgroup: create root hierarchy and setup controllers
    poe_cgroup_init();

    // TODO: CLONE_NEWUSER
    pid_t pid = (pid_t)syscall(SYS_clone, SIGCHLD|CLONE_NEWIPC|CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWUTS|CLONE_NEWNET, 0);
    if (pid < 0) {
        ERROR("clone failed");
    } else if (!pid) {
        dup2(stdout_fd[1], STDOUT_FILENO);
        close(stdout_fd[0]);
        close(stdout_fd[1]);
        dup2(stderr_fd[1], STDERR_FILENO);
        close(stderr_fd[0]);
        close(stderr_fd[1]);
        close(errfd[0]); // errfd[1] will be closed by O_CLOEXEC

        poe_do_child(root, cmdl, errfd[1]);
    } else {
        int epoll_fd = epoll_create1(0);
        C_SYSCALL(epoll_fd);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGHUP);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        int signal_fd = signalfd(-1, &mask, 0);
        C_SYSCALL(signal_fd);

        int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
        C_SYSCALL(timer_fd);
        C_SYSCALL(timerfd_settime(timer_fd, 0, &(struct itimerspec) { .it_value.tv_sec = POE_TIME_LIMIT }, NULL));

#define ADD(_fd__) C_SYSCALL(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd__, &(struct epoll_event) { .data.fd = _fd__, .events = EPOLLIN }))
        ADD(signal_fd);
        ADD(timer_fd);
        ADD(errfd[0]);
        ADD(stdout_fd[0]);
        ADD(stderr_fd[0]);

        C_SYSCALL(ptrace(PTRACE_SEIZE, pid, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACESECCOMP | PTRACE_O_TRACEVFORK));

        poe_cgroup_add(pid);
        if (atexit(poe_cgroup_destroy)) ERROR("atexit (cgroup) failed");
        if (atexit(poe_destroy_playground)) ERROR("atexit (playground) failed");

        while (true) {
            struct epoll_event events[10];
            int n = epoll_wait(epoll_fd, events, sizeof(events) / sizeof(events[0]), -1);
            C_SYSCALL(n);

            for (int i = 0; i < n; i++) {
                struct epoll_event *ev = &events[i];
                if (ev->events & EPOLLERR) {
                    // fd closed
                    if (ev->data.fd == errfd[0]) {
                        char buf[PIPE_BUF];
                        ssize_t nx = read(errfd[0], buf, sizeof(buf));
                        if (nx > 0) ERROR("child err: %s", strndupa(buf, nx));
                    }
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
                            ERROR("partial read signalfd");
                        handle_signal(pid, &si);
                    } else if (ev->data.fd == timer_fd) {
                        handle_timer(pid);
                    } else if (ev->data.fd == errfd[0]) {
                        // ignore
                        continue;
                    }
                }
            }
        }
    }

    UNREACHABLE();
}
