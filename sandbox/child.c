#include "sandbox.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

// 親プロセスへ通知するための fd
// 何かが書き込まれた場合、親はエラーが発生したとみなす
// 何も書き込まれずに閉じられた場合、正常に exec に移行したとみなす
static int errorfd_w;

// override ERROR function
// stop until signal: should be killed, never returns
#define ERROR(...) do { \
    dprintf(errorfd_w, __VA_ARGS__); \
    pause(); \
    abort(); \
} while (0)

void
poe_do_child(const char *root, char **cmd, int errorfd_w_)
{
    errorfd_w = errorfd_w_;
    if (syscall(SYS_getpid) != 1) ERROR("not in PID NS");

    // kill self if parent died
    C_SYSCALL(prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0));

    C_SYSCALL(mount("none", "/", NULL, MS_PRIVATE|MS_REC, NULL)); // mount --make-rprivate /
    C_SYSCALL(mount(root, root, "bind", MS_BIND|MS_REC, NULL));
    C_SYSCALL(chroot(root));
    C_SYSCALL(mount("none", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, NULL));
    // TODO: /dev/null, ...

    C_SYSCALL(sethostname(POE_HOSTNAME, strlen(POE_HOSTNAME)));
    struct passwd *pw = getpwnam(POE_USERNAME);
    if (!pw) ERROR("getpwnam() failed");

    C_SYSCALL(chdir("/tmp"));
    C_SYSCALL(setsid());
    gid_t grps[] = { POE_GID };
    C_SYSCALL(setgroups(1, grps)); // set supplementary group IDs
    C_SYSCALL(setresgid(POE_GID, POE_GID, POE_GID));
    C_SYSCALL(setresuid(POE_UID, POE_UID, POE_UID));

    // wait parent
    raise(SIGSTOP);

    C_SYSCALL(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
    poe_seccomp_init();

    char *env[] = {
        "PATH=/opt/bin:/usr/bin",
        "USER=" POE_USERNAME,
        "LOGNAME=" POE_USERNAME,
        "HOME=/tmp",
        NULL
    };

    // errorfd_w will be closed by O_CLOEXEC
    C_SYSCALL(execvpe(cmd[0], cmd, env));
}
