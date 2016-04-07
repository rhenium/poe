#include "sandbox.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

#define checked_syscall(s) do { \
	if ((s) < 0) \
		bug("CRITICAL: %s:%d %s", __FILE__, __LINE__, #s); \
} while (0)

noreturn void poe_child_do(struct playground *pg,
		int stdout_fd[2], int stderr_fd[2], int child_fd[2])
{
	if (dup2(child_fd[1], STDERR_FILENO) < 0)
		// 標準エラー出力に出ちゃうけどしかたない
		bug("dup2 child_fd_w to stdout failed");

	if (atexit(abort))
		bug("atexit failed");
	if (syscall(SYS_getpid) != 1)
		bug("not in PID NS");

	// kill self if parent died
	checked_syscall(prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0));

	checked_syscall(mount("none", "/", NULL, MS_PRIVATE|MS_REC, NULL)); // mount --make-rprivate /
	checked_syscall(mount(pg->mergeddir, pg->mergeddir, "bind", MS_BIND|MS_REC, NULL));
	checked_syscall(chroot(pg->mergeddir));
	checked_syscall(mount("none", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, NULL));
	// TODO: /dev/null, ...

	checked_syscall(sethostname(POE_HOSTNAME, strlen(POE_HOSTNAME)));
	struct passwd *pw = getpwnam(POE_USERNAME);
	if (!pw)
		bug("getpwnam() failed");

	checked_syscall(chdir(pw->pw_dir));
	checked_syscall(setsid());
	gid_t grps[] = { POE_GID };
	checked_syscall(setgroups(1, grps)); // set supplementary group IDs
	checked_syscall(setresgid(POE_GID, POE_GID, POE_GID));
	checked_syscall(setresuid(POE_UID, POE_UID, POE_UID));

	// wait parent
	raise(SIGSTOP);

	checked_syscall(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
	if (poe_seccomp_init())
		bug("seccomp init failed");

	// child_fd は exec によって close される
	if (dup2(stdout_fd[1], STDOUT_FILENO) < 0 || close(stdout_fd[0]) || close(stdout_fd[1]))
		bug("dup2/close stdout failed");
	if (dup2(stderr_fd[1], STDERR_FILENO) < 0 || close(stderr_fd[0]) || close(stderr_fd[1]))
		bug("dup2/close stderr failed");

	char *const env[] = {
		"PATH=/usr/bin",
		"USER=" POE_USERNAME,
		"LOGNAME=" POE_USERNAME,
		"HOME=/tmp",
		NULL
	};

	checked_syscall(execvpe(pg->command_line[0], pg->command_line, env));

	bug("unreachable");
}
