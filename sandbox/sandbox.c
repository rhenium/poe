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
child(const char *root, int cmdl, char *cmd[], const char *prog)
{
    pid_t pid = (pid_t)syscall(SYS_getpid);
    assert(pid == 1);

    // die when parent dies
    if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) == -1) ERROR("prctl(PR_SET_PDEATHSIG, SIGKILL) failed");

    if (sethostname(POE_HOSTNAME, strlen(POE_HOSTNAME)) == -1) ERROR("sethostname() failed");
    if (mount(NULL, "/",        NULL,           MS_PRIVATE | MS_REC, NULL) == -1) ERROR("mount / failed");
    if (mount(root, root,       "bind",         MS_BIND | MS_REC, NULL) == -1) ERROR("bind root failed");
    if (chroot(root) == -1) ERROR("chroot() failed");
    // if (mount(NULL, "/proc",    "proc",         MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) == -1) ERROR("mount /proc failed");
    // if (mount(NULL, "/dev",     "devtmpfs",     MS_NOSUID | MS_NOEXEC, NULL) == -1) ERROR("mount /dev failed");
    // if (mount(NULL, "/dev/shm", "tmpfs",        MS_NOSUID | MS_NODEV, NULL) == -1) ERROR("mount /dev/shm failed");

    struct passwd *pw = getpwnam(POE_USERNAME);
    if (!pw) ERROR("getpwnam() failed");

    if (mount(NULL, pw->pw_dir, "tmpfs", MS_NOSUID | MS_NODEV, NULL) == -1) ERROR("mount home failed");
    if (chdir("/tmp") == -1) ERROR("chdir(/tmp) failed");
    if (setsid() == -1) ERROR("setsid() failed");
    if (initgroups(POE_USERNAME, pw->pw_gid) == -1) ERROR("initgroups() failed");
    if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1) ERROR("setresgid() failed");
    if (setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1) ERROR("setresuid() failed");

    char *env[] = {
        "PATH=/opt/bin:/usr/bin",
        "USER=" POE_USERNAME,
        "LOGNAME=" POE_USERNAME,
        NULL,
        NULL
    };

    if (asprintf(env + 3, "HOME=%s", pw->pw_dir) == -1) ERROR("asprintf() failed");

    for (int i = 0; i < cmdl; i++) {
        if (!strcmp(cmd[i], "PROGRAM")) {
            cmd[i] = (char *)prog;
        }
    }

    // wait parent
    if (kill(pid, SIGSTOP) == -1) ERROR("kill(self, SIGSTOP) failed");

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) ERROR("ptctl(PR_SET_NO_NEW_PRIVS, 1) failed");
    poe_init_seccomp(SCMP_ACT_TRACE(0));

    if (execvpe(cmd[0], cmd, env) == -1) ERROR("execvpe() failed");
}

static inline long
get_arg(pid_t pid, int i)
{
    static const int regs[] = {RDI, RSI, RDX, R10, R8, R9};
    return ptrace(PTRACE_PEEKUSER, pid, sizeof(intptr_t) * (size_t)regs[i - 1]);
}

static enum poe_handler_result
handle_syscall(pid_t pid, int syscalln) {
    long arg1;
    switch (syscalln) {
    case SYS_write:
        arg1 = get_arg(pid, 1);
        if (arg1 == 1 || arg1 == 2) {
            char *pp = (char *)get_arg(pid, 2);
            int count = (int)get_arg(pid, 3);
            char fd = (char)arg1;
            fwrite(&fd, sizeof(fd), 1, stdout);
            fwrite(&count, sizeof(count), 1, stdout);
            for (int k = 0; k < count; k++, pp++) {
                char d = (char)ptrace(PTRACE_PEEKDATA, pid, pp);
                fwrite(&d, sizeof(d), 1, stdout);
            }
            ptrace(PTRACE_POKEUSER, pid, sizeof(long) * RAX, count);
            return POE_HANDLED;
        } else {
            return POE_ALLOWED;
        }
        break;
    }
    return POE_PROHIBITED;
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
        if (spid == -1) ERROR("waitpid() failed");
        if (!spid) break;

        if (WIFEXITED(status) && spid == mpid) {
            FINISH(POE_SUCCESS, WEXITSTATUS(status), NULL);
        } else if (WIFSIGNALED(status)) {
            FINISH(POE_SIGNALED, -1, "Program terminated with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
        } else if (WIFSTOPPED(status)) {
            int e = status >> 16 & 0xff;
            switch (e) {
            case PTRACE_EVENT_SECCOMP:
                errno = 0;
                int syscalln = ptrace(PTRACE_PEEKUSER, spid, sizeof(long) * ORIG_RAX);
                if (errno) ERROR("ptrace(PTRACE_PEEKUSER, ) failed");
                enum poe_handler_result ret = handle_syscall(spid, syscalln);
                if (ret == POE_HANDLED) {
                    // cancel syscall
                    ptrace(PTRACE_POKEUSER, spid, sizeof(long) * ORIG_RAX, -1);
                } else if (ret == POE_PROHIBITED) {
                    // implicitly prohibited syscall
                    kill(spid, SIGKILL);
                    char *rule = seccomp_syscall_resolve_num_arch(SCMP_ARCH_NATIVE, syscalln);
                    if (!rule) ERROR("seccomp_syscall_resolve_num_arch() failed");
                    FINISH(POE_SIGNALED, -1, "System call %s is blocked", rule);
                }
                ptrace(PTRACE_CONT, spid, 0, 0);
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
timer_handler(sd_event_source *es, uint64_t usec, void *vmpid)
{
    (void)es; (void)usec; (void)vmpid;
    FINISH(POE_TIMEDOUT, -1, NULL);
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        ERROR("usage: %s baseroot envroot program cmdl..", program_invocation_short_name);
    }

    int rc;
    const char *root = poe_init_playground(argv[1], argv[2]);
    const char *prog = copy_program(root, argv[3]);

    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);

    // TODO: CLONE_NEWUSER
    pid_t pid = (pid_t)syscall(SYS_clone, SIGCHLD | CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNET, 0);
    if (pid == -1) {
        ERROR("clone() failed");
    } else if (pid == 0) {
        sigprocmask(SIG_SETMASK, &omask, NULL);
        child(root, argc - 4, argv + 4, prog);
    } else {
        sd_event *event = NULL;

        rc = sd_event_default(&event);
        if (rc < 0) ERROR("sd_event_default() failed");
        rc = sd_event_add_signal(event, NULL, SIGCHLD, sigchld_handler, &pid);
        if (rc < 0) ERROR("sd_event_add_signal() failed");
        uint64_t now;
        rc = sd_event_now(event, CLOCK_MONOTONIC, &now);
        if (rc < 0) ERROR("sd_event_now() failed");
        rc = sd_event_add_time(event, NULL, CLOCK_MONOTONIC, now + POE_TIME_LIMIT, 0, timer_handler, &pid);
        if (rc < 0) ERROR("sd_event_add_time() failed");
        rc = ptrace(PTRACE_SEIZE, pid, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACESECCOMP | PTRACE_O_TRACEVFORK);
        if (rc < 0) ERROR("ptrace(PTRACE_SEIZE, ) failed");

        poe_init_systemd(pid);

        rc = sd_event_loop(event);
        if (rc < 0) ERROR("sd_event_loop() failed");
    }

    ERROR("unreachable");
}
