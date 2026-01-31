/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_file File node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node that matches and completes file paths.
 */

#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include <ecoli/node.h>

/** @internal below functions pointers are only useful for test */
struct ec_node_file_ops {
	int (*lstat)(const char *pathname, struct stat *buf);
	DIR *(*opendir)(const char *name);
	struct dirent *(*readdir)(DIR *dirp);
	int (*closedir)(DIR *dirp);
	int (*dirfd)(DIR *dirp);
	int (*fstatat)(int dirfd, const char *pathname, struct stat *buf, int flags);
};

/**
 * Set custom file operations for testing.
 * @internal
 */
void ec_node_file_set_ops(const struct ec_node_file_ops *ops);

/** @} */
