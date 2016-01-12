#include "sandbox.h"

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

    // die when parent dies
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

    NONNEGATIVE(mount(NULL, pw->pw_dir, "tmpfs", MS_NOSUID | MS_NODEV, NULL));
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
    poe_init_seccomp(SCMP_ACT_TRACE(0));

    NONNEGATIVE(execvpe(cmd[0], cmd, env));
}

static inline long
get_arg(pid_t pid, int i)
{
    static const int regs[] = {RDI, RSI, RDX, R10, R8, R9};
    return ptrace(PTRACE_PEEKUSER, pid, sizeof(intptr_t) * (size_t)regs[i - 1]);
}

static void
handle_syscall(pid_t pid)
{
    errno = 0;
    int syscalln = ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * ORIG_RAX);
    if (errno) ERROR("ptrace(PTRACE_PEEKUSER) failed");

    switch (syscalln) {
    default:
        goto kill;
    }
kill:
    kill(pid, SIGKILL);
    char *rule = seccomp_syscall_resolve_num_arch(SCMP_ARCH_NATIVE, syscalln);
    if (!rule) ERROR("seccomp_syscall_resolve_num_arch() failed");
    FINISH(POE_SIGNALED, -1, "System call %s is blocked", rule);
allowed:
    ptrace(PTRACE_CONT, pid, 0, 0);
}

static const char *
copy_program(const char *root, const char *progpath)
{
    FILE *src = fopen(progpath, "rb");
    if (!src) ERROR("could not open src");

    const char *newrel = "/tmp/prog";
    char * fullpath = (char *)malloc(strlen(root) + strlen(newrel) + 1);
    if (!fullpath) ERROR("failed malloc");
    strcpy(fullpath, root);
    strcat(fullpath, newrel);

    FILE *dst = fopen(fullpath, "wb");
    if (!dst) ERROR("could not open dst");

    char buf[1024];
    int n;
    while ((n = fread(buf, sizeof(buf[0]), sizeof(buf), src)) > 0) {
        if (fwrite(buf, sizeof(buf[0]), n, dst) == 0)
            ERROR("failed fwrite");
    }

    fclose(dst);
    fclose(src);

    chmod(fullpath, 0644);

    return newrel;
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
        } else if (WIFSIGNALED(status)) {
            FINISH(POE_SIGNALED, -1, "Program terminated with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
        } else if (WIFSTOPPED(status)) {
            int e = status >> 16 & 0xff;
            switch (e) {
            case PTRACE_EVENT_SECCOMP:
                handle_syscall(spid);
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
    int orig_fd = *(int *)vorig_fd;
    char buf[BUFSIZ];
    int n;

    n = (int)read(fd, buf, sizeof(buf));
    if (n < 0) {
        if (errno == EAGAIN) {
            return 0;
        } else {
            NONNEGATIVE(n);
        }
    } else {
        NONNEGATIVE(write(STDOUT_FILENO, &orig_fd, sizeof(orig_fd)));
        NONNEGATIVE(write(STDOUT_FILENO, &n, sizeof(n)));
        NONNEGATIVE(write(STDOUT_FILENO, buf, (size_t)n));
    }

    return 0;
}

static char **
construct_cmdl(int cmdl, char *cmd[], const char *prog)
{
    for (int i = 0; i < cmdl; i++) {
        if (!strcmp(cmd[i], "PROGRAM")) {
            cmd[i] = (char *)prog;
        }
    }

    return cmd;
}

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        ERROR("usage: %s baseroot envroot program cmdl..", program_invocation_short_name);
    }

    const char *root = poe_init_playground(argv[1], argv[2]);
    const char *prog = copy_program(root, argv[3]);
    char **cmdl = construct_cmdl(argc - 4, argv + 4, prog);

    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT); // auau
    sigaddset(&mask, SIGTERM); // auau
    sigprocmask(SIG_BLOCK, &mask, &omask);

    int stdout_fd[2], stderr_fd[2];
    NONNEGATIVE(pipe(stdout_fd));
    NONNEGATIVE(pipe(stderr_fd));

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

        sigprocmask(SIG_SETMASK, &omask, NULL);
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

    ERROR("unreachable");
}
