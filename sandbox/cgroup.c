#include "sandbox.h"
#include <mntent.h>

static char *cgroup_created = NULL;

static bool
cgroup_find(void)
{
    struct mntent *e;
    FILE* mtab = setmntent("/etc/mtab", "r");
    if (!mtab) ERROR("failed to open /etc/mtab");
    while ((e = getmntent(mtab))) {
        if (!strcmp(e->mnt_dir, POE_CGROUP_ROOT)) {
            /* something is mounted */
            endmntent(mtab);
            if (strcmp(e->mnt_type, "cgroup2")) /* is not cgroup2 */
                ERROR("other filesystem is mounted");
            return true;
        }
    }
    endmntent(mtab);
    return false;
}

static void
cgroup_config(const char *template, const char *name, const char *str)
{
    char filename[strlen(template) + strlen(name) + 2];
    snprintf(filename, sizeof(filename), "%s/%s", template, name);
    int fd = open(filename, O_TRUNC|O_WRONLY);
    if (fd == -1) ERROR("open cgroup property: %s", filename);
    if (write(fd, str, strlen(str)) == -1) ERROR("write cgroup error (val)");
    //if (write(fd, "\n", strlen("\n")) == -1) ERROR("write cgroup error (nl)");
    close(fd);
}

static void
cgroup_config_int(const char *template, const char *name, unsigned long long val)
{
    char buf[21]; // 0xffffffffffffffff.to_s.size #=> 20
    snprintf(buf, sizeof(buf), "%llu", val);
    cgroup_config(template, name, buf);
}

void
poe_cgroup_init(void)
{
    if (!cgroup_find()) {
        /* not mounted yet */
        struct stat sb;
        if (stat(POE_CGROUP_ROOT, &sb)) {
            if (errno != ENOENT) ERROR("cgroup root dir not accessible");
            /* if not exist */
            if (mkdir(POE_CGROUP_ROOT, 0755)) ERROR("can't create cgroup root dir");
        }
        if (mount("poe-cgroup2", POE_CGROUP_ROOT, "cgroup2", 0, NULL))
            ERROR("can't mount cgroup root dir");
    }

    /* then, configure it */
    cgroup_config(POE_CGROUP_ROOT, "cgroup.subtree_control", "+memory +pids");

    /* create sub group */
    struct stat sb;
    if (stat(POE_CGROUP_ROOT"/poe", &sb)) {
        if (errno == ENOENT) {
            if (mkdir(POE_CGROUP_ROOT"/poe"))
                ERROR("can't create sub cgroup");
        } else {
            ERROR("can't access sub cgroup");
        }
    }
}

void
poe_cgroup_add(pid_t pid)
{
    assert(!cgroup_created);
    char *template = strdup(POE_CGROUP_ROOT"/poe/poe-sandbox-XXXXXX");
    if (!mkdtemp(template)) ERROR("child cgroup create failed");
    cgroup_config_int(template, "cgroup.procs", pid);
    // enable this when Linux 4.6 released, which will include cpu controller
    // cgroup_config_int(template, "cpu.shares", 512);
    cgroup_config_int(template, "memory.max", POE_MEMORY_LIMIT);
    cgroup_config_int(template, "memory.swap.max", POE_MEMORY_LIMIT);
    cgroup_config_int(template, "pids.max", POE_TASKS_LIMIT);

    cgroup_created = template; /* must be freed */
}

void
poe_cgroup_destroy(void)
{
    struct stat sb;
    if (stat(cgroup_created, &sb))
        ERROR("cgroup is already destroyed?");
    if (rmdir(cgroup_created) == -1)
        ERROR("cgroup rmdir error");
    free(cgroup_created);
    cgroup_created = NULL;
}

void
poe_cgroup_unmount(void)
{
    assert(!cgroup_created);
    struct stat sb;
    if (stat(POE_CGROUP_ROOT"/poe", &sb))
        ERROR("can't access sub cgroup");
    if (rmdir(POE_CGROUP_ROOT"/poe"))
        ERROR("can't rmdir sub cgroup");
    if (umount(POE_CGROUP_ROOT) == -1)
        ERROR("cgroup umount error: %s", strerror(errno));
}
