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
        C_SYSCALL(mkdir(POE_TEMPORARY_BASE, 0755));
    }

    // setup base (tmpfs)
    C_SYSCALL(asprintf(&workbase, POE_TEMPORARY_BASE "/%ld", (long)getpid()));
    if (stat(workbase, &s) != -1) {
        C_SYSCALL(rmrf(workbase));
    }
    C_SYSCALL(mkdir(workbase, 0755));
    C_SYSCALL(mount(NULL, workbase, "tmpfs", MS_NOSUID, "size=32m")); // TODO

    C_SYSCALL(asprintf(&workdir, "%s/work", workbase));
    C_SYSCALL(mkdir(workdir, 0755));

    C_SYSCALL(asprintf(&upperdir, "%s/upper", workbase));
    C_SYSCALL(mkdir(upperdir, 0755));

    C_SYSCALL(asprintf(&mergeddir, "%s/merged", workbase));
    C_SYSCALL(mkdir(mergeddir, 0755));

    char *opts = NULL;
    C_SYSCALL(asprintf(&opts, "lowerdir=%s:%s,upperdir=%s,workdir=%s", env, base, upperdir, workdir));
    C_SYSCALL(mount(NULL, mergeddir, "overlay", MS_NOSUID, opts));

    return mergeddir;
}

char *
poe_copy_program_to_playground(const char *root, const char *progpath)
{
    FILE *src = fopen(progpath, "rb");
    if (!src) ERROR("could not open src");

    char *newrel = "/tmp/prog";
    char fullpath[strlen(root) + strlen(newrel) + 1];
    strcpy(fullpath, root);
    strcat(fullpath, newrel);

    FILE *dst = fopen(fullpath, "wb");
    if (!dst) ERROR("could not open dst");

    char buf[1024];
    int n;
    while ((n = fread(buf, sizeof(buf[0]), sizeof(buf), src)) > 0) {
        if (!fwrite(buf, sizeof(buf[0]), n, dst)) ERROR("failed fwrite");
    }

    fclose(dst);
    fclose(src);
    chmod(fullpath, 0644);

    return newrel;
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
