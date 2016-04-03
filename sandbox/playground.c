#include "sandbox.h"

static int rmrf_cb(const char *fpath, unused const struct stat *sb,
		unused int typeflag, unused struct FTW *ftwbuf)
{
	return remove(fpath);
}

static int rmrf(const char *path)
{
	return nftw(path, rmrf_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static void destroy_playground(unused int status, void *pg_)
{
	struct playground *pg = (struct playground *)pg_;
	struct stat s;

	if (pg->mergeddir) {
		umount(pg->mergeddir);
		rmrf(pg->mergeddir);
	}
	if (pg->workdir && !stat(pg->workdir, &s)) {
		rmrf(pg->workdir);
	}
	if (pg->upperdir && !stat(pg->upperdir, &s)) {
		rmrf(pg->upperdir);
	}
	if (pg->workbase && !stat(pg->workbase, &s)) {
		umount(pg->workbase);
		rmrf(pg->workbase);
	}

	free(pg->workbase);
	free(pg->upperdir);
	free(pg->workdir);
	free(pg->mergeddir);
	// command_line は free しない
	free(pg);
}

struct playground *poe_playground_init(const char *base, const char *env)
{
	struct playground *pg = xcalloc(1, sizeof(struct playground));
	if (on_exit(destroy_playground, pg))
		bug("on_exit failed");

	struct stat s;
	if (stat(POE_TEMPORARY_BASE, &s) && mkdir(POE_TEMPORARY_BASE, 0755)) {
		error("mkdir temporary base error");
		return NULL;
	}

	// setup base (tmpfs)
	pg->workbase = xformat(POE_TEMPORARY_BASE"/%ld", (long)getpid());
	if (!stat(pg->workbase, &s) && rmrf(pg->workbase)) {
		error("workbase cleanup failed");
		return NULL;
	}
	if (mkdir(pg->workbase, 0755) ||
			mount(NULL, pg->workbase, "tmpfs", MS_NOSUID, "size=32m")) {
		error("mount workbase failed");
		return NULL;
	}

	pg->workdir = xformat("%s/work", pg->workbase);
	if (mkdir(pg->workdir, 0755)) {
		error("mkdir workdir failed");
		return NULL;
	}

	pg->upperdir = xformat("%s/upper", pg->workbase);
	if (mkdir(pg->upperdir, 0755)) {
		error("mkdir upperdir failed");
		return NULL;
	}

	pg->mergeddir = xformat("%s/merged", pg->workbase);
	if (mkdir(pg->mergeddir, 0755)) {
		error("mkdir mergeddir failed");
		return NULL;
	}

	char *opts = xformat("lowerdir=%s:%s,upperdir=%s,workdir=%s",
			env, base, pg->upperdir, pg->workdir);
	if (mount(NULL, pg->mergeddir, "overlay", MS_NOSUID, opts)) {
		free(opts);
		error("mount overlay failed");
		return NULL;
	}
	free(opts);

	return pg;
}

int poe_playground_init_command_line(struct playground *pg, char **cmdl, const char *progpath)
{
	int src = open(progpath, O_RDONLY);
	if (src < 0)
		return error("could not open src");

	const char *prog_rel = "/tmp/prog";
	char *fullpath = xformat("%s%s", pg->mergeddir, prog_rel);
	int dst = open(fullpath, O_RDWR | O_CREAT, 0644);
	free(fullpath);
	if (dst < 0) {
		close(src);
		return error("could not open dst");
	}

	struct stat sb;
	if (fstat(src, &sb)) {
		close(src);
		close(dst);
		return error("could not fstat src");
	}

	if (sendfile(dst, src, NULL, sb.st_size) != sb.st_size) {
		close(src);
		close(dst);
		return error("could not copy src to dst");
	}

	if (fchown(dst, POE_UID, POE_GID)) {
		close(src);
		close(dst);
		return error("could not chown dst");
	}

	close(src);
	close(dst);

	char **curr = cmdl;
	do if (!strcmp("{}", *curr)) *curr = xstrdup(prog_rel); while (*++curr);
	pg->command_line = cmdl;

	return 0;
}
