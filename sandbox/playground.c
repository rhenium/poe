#include "sandbox.h"

static char *upperdir = NULL;
static char *workdir = NULL;
static char *mergeddir = NULL;

static int
rmrf_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void)sb; (void)typeflag; (void)ftwbuf;
    return remove(fpath);
}

static int
rmrf(const char *path)
{
    return nftw(path, rmrf_cb, 64, FTW_DEPTH | FTW_PHYS);
}

char *
poe_init_playground(const char *base, const char *env)
{
    struct stat s;
    if (stat(POE_TEMPORARY_BASE, &s) == -1) {
        NONNEGATIVE(mkdir(POE_TEMPORARY_BASE, 0755));
    }

    workdir = strdup(POE_WORKDIR_TEMPLATE);
    if (!workdir || !mkdtemp(workdir)) ERROR("failed to create workdir");
    NONNEGATIVE(chmod(workdir, 0755));
    upperdir = strdup(POE_UPPERDIR_TEMPLATE);
    if (!upperdir || !mkdtemp(upperdir)) ERROR("failed to create upperdir");
    NONNEGATIVE(chmod(upperdir, 0755));
    mergeddir = strdup(POE_MERGEDDIR_TEMPLATE);
    if (!mergeddir || !mkdtemp(mergeddir)) ERROR("failed to create mergeddir");
    NONNEGATIVE(chmod(mergeddir, 0755));

    char *opts = NULL;
    NONNEGATIVE(asprintf(&opts, "lowerdir=%s:%s,upperdir=%s,workdir=%s", env, base, upperdir, workdir));
    NONNEGATIVE(mount(NULL, mergeddir, "overlay", MS_NOSUID, opts));

    return mergeddir;
}

void
poe_destroy_playground()
{
    struct stat s;
    if (mergeddir) {
        umount(mergeddir);
        rmrf(mergeddir);
        free(mergeddir);
    }
    if (workdir && stat(workdir, &s) != -1) {
        rmrf(workdir);
        free(workdir);
    }
    if (upperdir && stat(upperdir, &s) != -1) {
        rmrf(upperdir);
        free(upperdir);
    }
}
