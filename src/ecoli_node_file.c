/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <ecoli_log.h>
#include <ecoli_malloc.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_string.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_file.h>

EC_LOG_TYPE_REGISTER(node_file);

struct ec_node_file {
	/* below functions pointers are only useful for test */
	int (*lstat)(const char *pathname, struct stat *buf);
	DIR *(*opendir)(const char *name);
	struct dirent *(*readdir)(DIR *dirp);
	int (*closedir)(DIR *dirp);
	int (*dirfd)(DIR *dirp);
	int (*fstatat)(int dirfd, const char *pathname, struct stat *buf,
		int flags);
};

static int
ec_node_file_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	(void)node;
	(void)pstate;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	return 1;
}

/*
 * Almost the same than dirname (3) and basename (3) except that:
 * - it always returns a substring of the given path, which can
 *   be empty.
 * - the behavior is different when the path finishes with a '/'
 * - the path argument is not modified
 * - the outputs are allocated and must be freed with ec_free().
 *
 *   path       dirname   basename       split_path
 *   /usr/lib   /usr      lib          /usr/     lib
 *   /usr/      /         usr          /usr/
 *   usr        .         usr                    usr
 *   /          /         /            /
 *   .          .         .                      .
 *   ..         .         ..                     ..
 */
static int split_path(const char *path, char **dname_p, char **bname_p)
{
	char *last_slash;
	size_t dirlen;
	char *dname, *bname;

	*dname_p = NULL;
	*bname_p = NULL;

	last_slash = strrchr(path, '/');
	if (last_slash == NULL)
		dirlen = 0;
	else
		dirlen = last_slash - path + 1;

	dname = ec_strdup(path);
	if (dname == NULL)
		return -1;
	dname[dirlen] = '\0';

	bname = ec_strdup(path + dirlen);
	if (bname == NULL) {
		ec_free(dname);
		return -1;
	}

	*dname_p = dname;
	*bname_p = bname;

	return 0;
}

static int
ec_node_file_complete(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_node_file *priv = ec_node_priv(node);
	char *dname = NULL, *bname = NULL, *effective_dir;
	struct ec_comp_item *item = NULL;
	enum ec_comp_type type;
	struct stat st, st2;
	const char *input;
	size_t bname_len;
	struct dirent *de = NULL;
	DIR *dir = NULL;
	char *comp_str = NULL;
	char *disp_str = NULL;
	int is_dir = 0;

	/*
	 * Example with this file tree:
	 * /
	 * ├── dir1
	 * │   ├── file1
	 * │   ├── file2
	 * │   └── subdir
	 * │       └── file3
	 * ├── dir2
	 * │   └── file4
	 * └── file5
	 *
	 * Input     Output completions
	 *   /       [dir1/, dir2/, file5]
	 *   /d      [dir1/, dir2/]
	 *   /f      [file5]
	 *   /dir1/  [file1, file2, subdir/]
	 *
	 *
	 *
	 */

	if (ec_strvec_len(strvec) != 1)
		return 0;

	input = ec_strvec_val(strvec, 0);
	if (split_path(input, &dname, &bname) < 0)
		return -1;

	if (strcmp(dname, "") == 0)
		effective_dir = ".";
	else
		effective_dir = dname;

	if (priv->lstat(effective_dir, &st) < 0)
		goto out;
	if (!S_ISDIR(st.st_mode))
		goto out;

	dir = priv->opendir(effective_dir);
	if (dir == NULL)
		goto out;

	bname_len = strlen(bname);
	while (1) {
		de = priv->readdir(dir);
		if (de == NULL)
			goto out;

		if (!ec_str_startswith(de->d_name, bname))
			continue;
		if (bname[0] != '.' && de->d_name[0] == '.')
			continue;

		/* add '/' if it's a dir */
		if (de->d_type == DT_DIR) {
			is_dir = 1;
		} else if (de->d_type == DT_UNKNOWN) {
			int dir_fd = priv->dirfd(dir);

			if (dir_fd < 0)
				goto out;
			if (priv->fstatat(dir_fd, de->d_name, &st2, 0) < 0)
				goto out;
			if (S_ISDIR(st2.st_mode))
				is_dir = 1;
			else
				is_dir = 0;
		} else {
			is_dir = 0;
		}

		if (is_dir) {
			type = EC_COMP_PARTIAL;
			if (ec_asprintf(&comp_str, "%s%s/", input,
					&de->d_name[bname_len]) < 0)
				goto fail;
			if (ec_asprintf(&disp_str, "%s/", de->d_name) < 0)
				goto fail;
		} else {
			type = EC_COMP_FULL;
			if (ec_asprintf(&comp_str, "%s%s", input,
					&de->d_name[bname_len]) < 0)
				goto fail;
			if (ec_asprintf(&disp_str, "%s", de->d_name) < 0)
				goto fail;
		}
		item = ec_comp_add_item(comp, node, type, input, comp_str);
		if (item == NULL)
			goto fail;

		/* fix the display string: we don't want to display the full
		 * path. */
		if (ec_comp_item_set_display(item, disp_str) < 0)
			goto fail;

		item = NULL;
		ec_free(comp_str);
		comp_str = NULL;
		ec_free(disp_str);
		disp_str = NULL;
	}
out:
	ec_free(comp_str);
	ec_free(disp_str);
	ec_free(dname);
	ec_free(bname);
	if (dir != NULL)
		priv->closedir(dir);

	return 0;

fail:
	ec_free(comp_str);
	ec_free(disp_str);
	ec_free(dname);
	ec_free(bname);
	if (dir != NULL)
		priv->closedir(dir);

	return -1;
}

static int
ec_node_file_init_priv(struct ec_node *node)
{
	struct ec_node_file *priv = ec_node_priv(node);

	priv->lstat = lstat;
	priv->opendir = opendir;
	priv->closedir = closedir;
	priv->readdir = readdir;
	priv->dirfd = dirfd;
	priv->fstatat = fstatat;

	return 0;
}

static struct ec_node_type ec_node_file_type = {
	.name = "file",
	.parse = ec_node_file_parse,
	.complete = ec_node_file_complete,
	.size = sizeof(struct ec_node_file),
	.init_priv = ec_node_file_init_priv,
};

EC_NODE_TYPE_REGISTER(ec_node_file_type);

/* LCOV_EXCL_START */
static int
test_lstat(const char *pathname, struct stat *buf)
{
	if (!strcmp(pathname, "/tmp/toto/")) {
		struct stat st = { .st_mode = S_IFDIR };
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	}

	errno = ENOENT;
	return -1;
}

static DIR *
test_opendir(const char *name)
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

static struct dirent *
test_readdir(DIR *dirp)
{
	static struct dirent de[] = {
		{ .d_type = DT_DIR, .d_name = ".." },
		{ .d_type = DT_DIR, .d_name = "." },
		{ .d_type = DT_REG, .d_name = "bar" },
		{ .d_type = DT_UNKNOWN, .d_name = "bar2" },
		{ .d_type = DT_REG, .d_name = "foo" },
		{ .d_type = DT_DIR, .d_name = "titi" },
		{ .d_type = DT_UNKNOWN, .d_name = "tutu" },
		{ .d_name = "" },
	};
	int *p = (int *)dirp;
	struct dirent *ret = &de[*p];

	if (!strcmp(ret->d_name, ""))
		return NULL;

	*p = *p + 1;

	return ret;
}

static int
test_closedir(DIR *dirp)
{
	free(dirp);
	return 0;
}

static int
test_dirfd(DIR *dirp)
{
	int *p = (int *)dirp;
	return *p;
}

static int
test_fstatat(int dirfd, const char *pathname, struct stat *buf,
	int flags)
{
	(void)dirfd;
	(void)flags;

	if (!strcmp(pathname, "bar2")) {
		struct stat st = { .st_mode = S_IFREG };
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	} else if (!strcmp(pathname, "tutu")) {
		struct stat st = { .st_mode = S_IFDIR };
		memcpy(buf, &st, sizeof(*buf));
		return 0;
	}

	errno = ENOENT;
	return -1;
}

static int
ec_node_file_override_functions(struct ec_node *node)
{
	struct ec_node_file *priv = ec_node_priv(node);

	priv->lstat = test_lstat;
	priv->opendir = test_opendir;
	priv->readdir = test_readdir;
	priv->closedir = test_closedir;
	priv->dirfd = test_dirfd;
	priv->fstatat = test_fstatat;

	return 0;
}

static int ec_node_file_testcase(void)
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node("file", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	ec_node_file_override_functions(node);

	/* any string matches */
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "/tmp/bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1);

	/* test completion */
	testres |= EC_TEST_CHECK_COMPLETE(node,
		EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"/tmp/toto/t", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE_PARTIAL(node,
		"/tmp/toto/t", EC_VA_END,
		"/tmp/toto/titi/", "/tmp/toto/tutu/", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"/tmp/toto/f", EC_VA_END,
		"/tmp/toto/foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"/tmp/toto/b", EC_VA_END,
		"/tmp/toto/bar", "/tmp/toto/bar2", EC_VA_END);

	ec_node_free(node);

	return testres;
}

static struct ec_test ec_node_file_test = {
	.name = "node_file",
	.test = ec_node_file_testcase,
};

EC_TEST_REGISTER(ec_node_file_test);
/* LCOV_EXCL_STOP */
