/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "test.h"

static int test_lstat(const char *pathname, struct stat *buf)
{
	if (!strcmp(pathname, "/tmp/toto/")) {
		struct stat st = {.st_mode = S_IFDIR};
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	}

	errno = ENOENT;
	return -1;
}

static DIR *test_opendir(const char *name)
{
	int *p;

	if (strcmp(name, "/tmp/toto/")) {
		errno = ENOENT;
		return NULL;
	}

	p = malloc(sizeof(int));
	if (p)
		*p = 0;

	return (DIR *)p;
}

static struct dirent *test_readdir(DIR *dirp)
{
	static struct dirent de[] = {
		{.d_type = DT_DIR, .d_name = ".."},
		{.d_type = DT_DIR, .d_name = "."},
		{.d_type = DT_REG, .d_name = "bar"},
		{.d_type = DT_UNKNOWN, .d_name = "bar2"},
		{.d_type = DT_REG, .d_name = "foo"},
		{.d_type = DT_DIR, .d_name = "titi"},
		{.d_type = DT_UNKNOWN, .d_name = "tutu"},
		{.d_name = ""},
	};
	int *p = (int *)dirp;
	struct dirent *ret = &de[*p];

	if (!strcmp(ret->d_name, ""))
		return NULL;

	*p = *p + 1;

	return ret;
}

static int test_closedir(DIR *dirp)
{
	free(dirp);
	return 0;
}

static int test_dirfd(DIR *dirp)
{
	int *p = (int *)dirp;
	return *p;
}

static int test_fstatat(int dirfd, const char *pathname, struct stat *buf, int flags)
{
	(void)dirfd;
	(void)flags;

	if (!strcmp(pathname, "bar2")) {
		struct stat st = {.st_mode = S_IFREG};
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	} else if (!strcmp(pathname, "tutu")) {
		struct stat st = {.st_mode = S_IFDIR};
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	}

	errno = ENOENT;
	return -1;
}

static struct ec_node_file_ops test_ops = {
	.lstat = test_lstat,
	.opendir = test_opendir,
	.readdir = test_readdir,
	.closedir = test_closedir,
	.dirfd = test_dirfd,
	.fstatat = test_fstatat,
};

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	ec_node_file_set_ops(&test_ops);

	node = ec_node("file", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}

	/* any string matches */
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "/tmp/bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1);

	/* test completion */
	testres |= EC_TEST_CHECK_COMPLETE(node, EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "/tmp/toto/t", EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE_PARTIAL(
		node, "/tmp/toto/t", EC_VA_END, "/tmp/toto/titi/", "/tmp/toto/tutu/", EC_VA_END
	);
	testres |= EC_TEST_CHECK_COMPLETE(
		node, "/tmp/toto/f", EC_VA_END, "/tmp/toto/foo", EC_VA_END
	);
	testres |= EC_TEST_CHECK_COMPLETE(
		node, "/tmp/toto/b", EC_VA_END, "/tmp/toto/bar", "/tmp/toto/bar2", EC_VA_END
	);

	ec_node_free(node);

	return testres;
}
