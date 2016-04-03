#ifndef CONFIG_H
#define CONFIG_H

#define POE_DEBUG 1

#define POE_TEMPORARY_BASE "/tmp/poe"
#define POE_CGROUP_ROOT POE_TEMPORARY_BASE"/cgroup"
#define POE_USERNAME "unyapoe"
#define POE_HOSTNAME "poe-sandbox"
#define POE_MEMORY_LIMIT (1024ULL * 1024ULL * 128ULL)
#define POE_TASKS_LIMIT 32
#define POE_TIME_LIMIT 5 // sec
#define POE_UID 27627
#define POE_GID 27627

#endif
