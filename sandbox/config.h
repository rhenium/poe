#ifndef CONFIG_H
#define CONFIG_H

#define POE_TEMPORARY_BASE "/tmp/poe"
#define POE_USERNAME "nobody"
#define POE_HOSTNAME "poe-sandbox"
#define POE_MEMORY_LIMIT (1024ULL * 1024ULL * 128ULL)
#define POE_TASKS_LIMIT 32ULL
#define POE_TIME_LIMIT (1000ULL * 1000ULL * 5ULL) // us

#endif
