#include "sandbox.h"

static char *upperdir = NULL;
static char *workdir = NULL;
static char *mergeddir = NULL;

char *
poe_init_playground(const char *base, const char *env)
{
    struct stat s;
    if (stat(POE_TEMPORARY_BASE, &s) == -1) {
        if (mkdir(POE_TEMPORARY_BASE, 0755) == -1) ERROR("failed to create temporary base");
    }

    workdir = strdup(POE_WORKDIR_TEMPLATE);
    if (!workdir || !mkdtemp(workdir)) ERROR("failed to create workdir");
    if (chmod(workdir, 0755) == -1) ERROR("failed to chmod workdir");
    upperdir = strdup(POE_UPPERDIR_TEMPLATE);
    if (!upperdir || !mkdtemp(upperdir)) ERROR("failed to create upperdir");
    if (chmod(upperdir, 0755) == -1) ERROR("failed to chmod upperdir");
    mergeddir = strdup(POE_MERGEDDIR_TEMPLATE);
    if (!mergeddir || !mkdtemp(mergeddir)) ERROR("failed to create mergeddir");
    if (chmod(mergeddir, 0755) == -1) ERROR("failed to chmod mergeddir");

    char *opts = NULL;
    if (asprintf(&opts, "lowerdir=%s:%s,upperdir=%s,workdir=%s", env, base, upperdir, workdir) == -1)
        ERROR("asprintf() failed");
    if (mount(NULL, mergeddir, "overlay", MS_NOSUID, opts) == -1)
        ERROR("mount overlay failed");

    return mergeddir;
}

void
poe_destroy_playground()
{
    struct stat s;
    if (mergeddir) {
        umount(mergeddir);
        if (rmdir(mergeddir) != -1) fprintf(stderr, "failed remove mergeddir");
        free(mergeddir);
    }
    if (workdir && stat(workdir, &s) != -1) {
        if (rmdir(workdir) != -1) fprintf(stderr, "failed remove workdir");
        free(workdir);
    }
    if (upperdir && stat(upperdir, &s) != -1) {
        fprintf(stderr, "remove upperdir");
        free(upperdir);
    }
}
