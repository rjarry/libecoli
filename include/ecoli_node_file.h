/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include <ecoli_node.h>

struct ec_node *ec_node_file(const char *id, const char *file);

/* file is duplicated */
int ec_node_file_set_str(struct ec_node *node, const char *file);

/** @internal below functions pointers are only useful for test */
struct ec_node_file_ops {
	int (*lstat)(const char *pathname, struct stat *buf);
	DIR *(*opendir)(const char *name);
	struct dirent *(*readdir)(DIR *dirp);
	int (*closedir)(DIR *dirp);
	int (*dirfd)(DIR *dirp);
	int (*fstatat)(int dirfd, const char *pathname, struct stat *buf, int flags);
};

/** @internal for tests */
void ec_node_file_set_ops(const struct ec_node_file_ops *ops);

/** @} */
