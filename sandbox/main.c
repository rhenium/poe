#include "sandbox.h"
#include <systemd/sd-event.h>
#include <sys/ptrace.h>

static void
poe_exit(int status)
{
    if (syscall(SYS_getpid) != 1) {
        poe_exit_systemd();
        poe_destroy_playground();
    }
    exit(status);
}

void
ERROR(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    poe_exit(1);
}

void
FINISH(enum poe_exit_reason reason, int status, const char *fmt, ...)
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
    poe_exit(0);
}

static void
child(const char *root, char *cmd[])
{
    pid_t pid = (pid_t)syscall(SYS_getpid);
    assert(pid == 1);

    NONNEGATIVE(prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0));

    NONNEGATIVE(sethostname(POE_HOSTNAME, strlen(POE_HOSTNAME)));
    NONNEGATIVE(mount(NULL, "/",        NULL,           MS_PRIVATE | MS_REC, NULL));
    NONNEGATIVE(mount(root, root,       "bind",         MS_BIND | MS_REC, NULL));
    NONNEGATIVE(chroot(root));
    // NONNEGATIVE(mount(NULL, "/proc",    "proc",         MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL));
    // NONNEGATIVE(mount(NULL, "/dev",     "devtmpfs",     MS_NOSUID | MS_NOEXEC, NULL));
    // NONNEGATIVE(mount(NULL, "/dev/shm", "tmpfs",        MS_NOSUID | MS_NODEV, NULL));

    struct passwd *pw = getpwnam(POE_USERNAME);
    if (!pw) ERROR("getpwnam() failed");

    NONNEGATIVE(chdir("/tmp"));
    NONNEGATIVE(setsid());
    NONNEGATIVE(initgroups(POE_USERNAME, pw->pw_gid));
    NONNEGATIVE(setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid));
    NONNEGATIVE(setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid));

    char *env[] = {
        "PATH=/opt/bin:/usr/bin",
        "USER=" POE_USERNAME,
        "LOGNAME=" POE_USERNAME,
        NULL,
        NULL
    };
    NONNEGATIVE(asprintf(env + 3, "HOME=%s", pw->pw_dir));

    // wait parent
    NONNEGATIVE(kill(pid, SIGSTOP));

    NONNEGATIVE(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
    poe_seccomp_init();

    NONNEGATIVE(execvpe(cmd[0], cmd, env));
}

static inline long
get_arg(pid_t pid, int i)
{
    static const int regs[] = {RDI, RSI, RDX, R10, R8, R9};
    return ptrace(PTRACE_PEEKUSER, pid, sizeof(intptr_t) * (size_t)regs[i - 1]);
}

static int
sigchld_handler(sd_event_source *es, const struct signalfd_siginfo *si, void *vmpid)
{
    (void)es;
    pid_t mpid = *(pid_t *)vmpid;
    if (si->ssi_signo != SIGCHLD) ERROR("parent: unexpected signal");

    while (true) {
        int status;
        pid_t spid = waitpid(-mpid, &status, WNOHANG | __WALL);
        NONNEGATIVE(spid);
        if (!spid) break;

        if (WIFEXITED(status) && spid == mpid) {
            FINISH(POE_SUCCESS, WEXITSTATUS(status), NULL);
        } else if (WIFSIGNALED(status) && spid == mpid) {
            FINISH(POE_SIGNALED, -1, "Program terminated with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
        } else if (WIFSTOPPED(status)) {
            unsigned long edata;
            int e = status >> 16 & 0xff;
            switch (e) {
            case PTRACE_EVENT_SECCOMP:
                NONNEGATIVE(ptrace(PTRACE_GETEVENTMSG, spid, 0, &edata));
                poe_seccomp_handle_syscall(spid, edata);
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
    return 0;
}

static int
sigint_handler(sd_event_source *es, const struct signalfd_siginfo *si, void *vmpid)
{
    (void)es;
    (void)si;
    (void)vmpid;
    FINISH(POE_TIMEDOUT, -1, "Supervisor terminated");
    return 0;
}

static int
timer_handler(sd_event_source *es, uint64_t usec, void *vmpid)
{
    (void)es;
    (void)usec;
    (void)vmpid;
    FINISH(POE_TIMEDOUT, -1, NULL);
    return 0;
}

static int
stdout_handler(sd_event_source *es, int fd, uint32_t revents, void *vorig_fd)
{
    (void)es;
    (void)revents;
    char orig_fd = (char)*(int *)vorig_fd;
    char buf[BUFSIZ];

    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        if (errno == EAGAIN) {
            return 0;
        } else {
            NONNEGATIVE(n);
        }
    } else {
        uint32_t len = (uint32_t)n;
        NONNEGATIVE(write(STDOUT_FILENO, &orig_fd, sizeof(orig_fd)));
        NONNEGATIVE(write(STDOUT_FILENO, &len, sizeof(len)));
        NONNEGATIVE(write(STDOUT_FILENO, buf, len));
    }

    return 0;
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
    if (argc < 5) {
        fprintf(stderr, "usage: runner basedir overlaydir sourcefile cmdl..\n");
        return EXIT_FAILURE;
    }

    char *root = poe_init_playground(argv[1], argv[2]);
    char *prog = poe_copy_program_to_playground(root, argv[3]);
    char **cmdl = construct_cmdl(argc - 4, argv + 4, prog);

    int stdout_fd[2], stderr_fd[2];
    NONNEGATIVE(pipe2(stdout_fd, O_DIRECT));
    NONNEGATIVE(pipe2(stderr_fd, O_DIRECT));

    // TODO: CLONE_NEWUSER
    pid_t pid = (pid_t)syscall(SYS_clone, SIGCHLD | CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNET, 0);
    NONNEGATIVE(pid);

    if (pid == 0) {
        dup2(stdout_fd[1], STDOUT_FILENO);
        close(stdout_fd[0]);
        close(stdout_fd[1]);
        dup2(stderr_fd[1], STDERR_FILENO);
        close(stderr_fd[0]);
        close(stderr_fd[1]);

        child(root, cmdl);
    } else {
        sd_event *event = NULL;
        uint64_t now;
        int stdout_fileno = STDOUT_FILENO;
        int stderr_fileno = STDERR_FILENO;

        int fflags;
        fflags = fcntl(stdout_fd[0], F_GETFL, 0);
        NONNEGATIVE(fflags);
        NONNEGATIVE(fcntl(stdout_fd[0], F_SETFL, fflags | O_NONBLOCK));
        fflags = fcntl(stderr_fd[0], F_GETFL, 0);
        NONNEGATIVE(fflags);
        NONNEGATIVE(fcntl(stderr_fd[0], F_SETFL, fflags | O_NONBLOCK));

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        NONNEGATIVE(sd_event_default(&event));
        NONNEGATIVE(sd_event_add_signal(event, NULL, SIGCHLD, sigchld_handler, &pid));
        NONNEGATIVE(sd_event_add_signal(event, NULL, SIGINT, sigint_handler, &pid));
        NONNEGATIVE(sd_event_add_signal(event, NULL, SIGTERM, sigint_handler, &pid));
        NONNEGATIVE(sd_event_now(event, CLOCK_MONOTONIC, &now));
        NONNEGATIVE(sd_event_add_time(event, NULL, CLOCK_MONOTONIC, now + POE_TIME_LIMIT, 0, timer_handler, &pid));
        NONNEGATIVE(sd_event_add_io(event, NULL, stdout_fd[0], EPOLLIN, stdout_handler, &stdout_fileno));
        NONNEGATIVE(sd_event_add_io(event, NULL, stderr_fd[0], EPOLLIN, stdout_handler, &stderr_fileno));

        NONNEGATIVE(ptrace(PTRACE_SEIZE, pid, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACESECCOMP | PTRACE_O_TRACEVFORK));

        poe_init_systemd(pid);

        NONNEGATIVE(sd_event_loop(event));
    }

    UNREACHABLE();
}
