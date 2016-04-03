#include "sandbox.h"
#include <mntent.h>

static void cgroup_config(const char *template, const char *name, const char *str)
{
	char filename[strlen(template) + strlen(name) + 2];
	snprintf(filename, sizeof(filename), "%s/%s", template, name);
	int fd = open(filename, O_TRUNC | O_WRONLY);
	if (fd == -1)
		bug("open cgroup property: %s", filename);
	if (write(fd, str, strlen(str)) == -1)
		bug("write cgroup error (val)");
	close(fd);
}

static void cgroup_config_int(const char *template, const char *name, uint64_t val)
{
	char buf[21]; // 0xffff_ffff_ffff_ffff.to_s.size #=> 20
	snprintf(buf, sizeof(buf), "%"PRIu64, val);
	cgroup_config(template, name, buf);
}

/// on_exit でよばれる
static void cgroup_destroy(unused int status, void *cgroup_)
{
	char *cgroup = (char *)cgroup_;
	struct stat sb;
	if (stat(cgroup, &sb))
		bug("cgroup does not exist?");

	if (rmdir(cgroup)) {
		// まだプロセスが生き残ってる？
		// pids.max を 0 にして fork を禁止してから SIGKILL るよ
		cgroup_config_int(cgroup, "pids.max", 0);
		char *procs_file = xformat("%s/cgroup.procs", cgroup);
		FILE *f = fopen(procs_file, "r");
		if (!f)
			bug("cgroup.procs open error");
		int spid;
		while (fscanf(f, "%d", &spid) != EOF)
			kill(spid, SIGKILL);
		while (waitpid(-1, NULL, __WALL) != -1);
		if (errno != ECHILD)
			bug("waitpid failed");
		fclose(f);
		if (rmdir(cgroup))
			bug("cgroup rmdir error");
	}
	free(cgroup);
}

warn_unused static int cgroup_find(bool *is_mounted)
{
	*is_mounted = false;

	FILE* mtab = setmntent("/etc/mtab", "r");
	if (!mtab)
		return error("failed to open /etc/mtab");

	struct mntent *e;
	while ((e = getmntent(mtab))) {
		if (strcmp(e->mnt_dir, POE_CGROUP_ROOT))
			continue;
		/* something is mounted */
		if (strcmp(e->mnt_type, "cgroup2")) /* is not cgroup2 */
			return error("other filesystem is mounted");
		*is_mounted = true;
		break;
	}

	endmntent(mtab);
	return 0;
}

warn_unused int poe_cgroup_init(void)
{
	bool is_mounted;
	if (cgroup_find(&is_mounted))
		return -1;
	if (!is_mounted) {
		struct stat sb;
		if (stat(POE_CGROUP_ROOT, &sb)) {
			if (errno != ENOENT)
				return error("cgroup root dir not accessible");
			/* if not exist */
			if (mkdir(POE_CGROUP_ROOT, 0755))
				return error("can't create cgroup root dir");
		}
		if (mount("poe-cgroup2", POE_CGROUP_ROOT, "cgroup2", 0, NULL))
			return error("can't mount cgroup root dir");
	}

	/* create sub group */
	struct stat sb;
	if (stat(POE_CGROUP_ROOT"/poe", &sb)) {
		if (errno != ENOENT)
			return error("can't access sub cgroup");

		if (mkdir(POE_CGROUP_ROOT"/poe", 0755))
			return error("can't create sub cgroup");
	}

	/* then, configure it */
	cgroup_config(POE_CGROUP_ROOT"/poe", "cgroup.subtree_control", "+memory +pids");

	// 他の poe-sandbox が使用するかもしれないので、マウントした cgroup は削除しない
	// どうしようこれ

	return 0;
}

warn_unused int poe_cgroup_add(pid_t pid)
{
	char *template = xstrdup(POE_CGROUP_ROOT"/poe/poe-sandbox-XXXXXX");
	if (!mkdtemp(template))
		return error("child cgroup create failed");
	cgroup_config_int(template, "cgroup.procs", pid);
	// enable this when Linux 4.6 released, which will include cpu controller
	// cgroup_config_int(template, "cpu.shares", 512);
	cgroup_config_int(template, "memory.max", POE_MEMORY_LIMIT);
	cgroup_config_int(template, "memory.swap.max", POE_MEMORY_LIMIT);
	cgroup_config_int(template, "pids.max", POE_TASKS_LIMIT);

	if (on_exit(cgroup_destroy, template))
		bug("on_exit for cgroup failed");

	return 0;
}
