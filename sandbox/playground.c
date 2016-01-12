#include "sandbox.h"

static char *workbase = NULL;
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

    // setup base (tmpfs)
    NONNEGATIVE(asprintf(&workbase, POE_TEMPORARY_BASE "/%ld", (long)getpid())); // pid_t is signed, not larger than long
    if (stat(workbase, &s) != -1) {
        NONNEGATIVE(rmrf(workbase));
    }
    NONNEGATIVE(mkdir(workbase, 0755));
    NONNEGATIVE(mount(NULL, workbase, "tmpfs", MS_NOSUID, "size=32m")); // TODO

    NONNEGATIVE(asprintf(&workdir, "%s/work", workbase));
    NONNEGATIVE(mkdir(workdir, 0755));

    NONNEGATIVE(asprintf(&upperdir, "%s/upper", workbase));
    NONNEGATIVE(mkdir(upperdir, 0755));

    NONNEGATIVE(asprintf(&mergeddir, "%s/merged", workbase));
    NONNEGATIVE(mkdir(mergeddir, 0755));

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
    if (workbase && stat(workbase, &s) != -1) {
        umount(workbase);
        rmrf(workbase);
        free(workbase);
    }
}
