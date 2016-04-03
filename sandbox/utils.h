#ifndef UTILS_H
#define UTILS_H

// errors
static inline int verror(const char *prefix, const char *fmt, va_list ap)
{
	int pref_len = prefix ? strlen(prefix) : 0;
	int fmt_len = strlen(fmt);

	char buf[pref_len + fmt_len + 2];
	if (prefix)
		strcpy(buf, prefix);
	strcpy(buf + pref_len, fmt);
	buf[pref_len + fmt_len] = '\n';
	buf[pref_len + fmt_len + 1] = '\0';

	vfprintf(stderr, buf, ap);
	return -1;
}

static inline int error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verror(NULL, fmt, args);
	va_end(args);
	return -1;
}

static inline noreturn void die(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verror(NULL, fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

static inline noreturn void bug(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verror("[BUG] ", fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

// memory utils
static inline void *_mem_nonnull(void *val)
{
	if (!val)
		die("not enough memory");
	return val;
}

static inline void *xmalloc(size_t size)
{
	return _mem_nonnull(malloc(size));
}

static inline void *xcalloc(size_t nmemb, size_t size)
{
	return _mem_nonnull(calloc(nmemb, size));
}

static inline char *xstrdup(const char *orig)
{
	return _mem_nonnull(strdup(orig));
}

static inline char *xformat(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *new = NULL;
	if (vasprintf(&new, fmt, args) == -1)
		_mem_nonnull(NULL);
	va_end(args);
	return new;
}

#endif
