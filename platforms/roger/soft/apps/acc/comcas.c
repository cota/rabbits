#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include "comcas.h"

static void remove_trailing_chars(char *path, char c)
{
	size_t len;

	if (path == NULL)
		return;
	len = strlen(path);
	while (len > 0 && path[len - 1] == c)
		path[--len] = '\0';
}

static void path_append(char *path, const char *append, size_t len)
{
	snprintf(path + strlen(path), len - strlen(path), "/%s", append);
	path[len - 1] = '\0';
}

static int __get_attr(const char *path, char *val, size_t len)
{
	struct stat statbuf;
	size_t size;
	int fd;

	/* make sure it's NUL-terminated even if we fail before filling it in */
	val[len - 1] = '\0';

	if (lstat(path, &statbuf) != 0) {
		fprintf(stderr, "warning: '%s' not found\n", path);
		return -1;
	}

	if (S_ISLNK(statbuf.st_mode))
		return -1;
	if (S_ISDIR(statbuf.st_mode))
		return -1;
	if (!(statbuf.st_mode & S_IRUSR))
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "warning: cannot open '%s'\n", path);
		return -1;
	}
	size = read(fd, val, len);
	val[len - 1] = '\0';
	if (close(fd) < 0)
		return -1;
	if (size < 0)
		return -1;
	if (size == len)
		return -1;

	val[size] = '\0';
	remove_trailing_chars(val, '\n');
	return 0;
}

int comcas_get_attr_u64(const char *path, uint64_t *valp)
{
	char value[256];
	int ret;

	ret = __get_attr(path, value, sizeof(value));
	if (ret)
		return ret;
	*valp = strtoull(value, NULL, 0);
	return 0;
}
