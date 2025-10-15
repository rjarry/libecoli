/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */


#include <ecoli_init.h>
#include <ecoli_malloc.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

EC_LOG_TYPE_REGISTER(malloc);

static int init_done = 0;

struct ec_malloc_handler ec_malloc_handler;

int ec_malloc_register(ec_malloc_t usr_malloc, ec_free_t usr_free,
	ec_realloc_t usr_realloc)
{
	if (usr_malloc == NULL || usr_free == NULL || usr_realloc == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (init_done) {
		errno = EBUSY;
		return -1;
	}

	ec_malloc_handler.malloc = usr_malloc;
	ec_malloc_handler.free = usr_free;
	ec_malloc_handler.realloc = usr_realloc;

	return 0;
}

void *__ec_malloc(size_t size, const char *file, unsigned int line)
{
	return ec_malloc_handler.malloc(size, file, line);
}

void *ec_malloc_func(size_t size)
{
	return ec_malloc(size);
}

void __ec_free(void *ptr, const char *file, unsigned int line)
{
	ec_malloc_handler.free(ptr, file, line);
}

void ec_free_func(void *ptr)
{
	ec_free(ptr);
}

void *__ec_calloc(size_t nmemb, size_t size, const char *file,
	unsigned int line)
{
	void *ptr;
	size_t total;

	/* check overflow */
	total = size * nmemb;
	if (nmemb != 0 && size != (total / nmemb)) {
		errno = ENOMEM;
		return NULL;
	}

	ptr = __ec_malloc(total, file, line);
	if (ptr == NULL)
		return NULL;

	memset(ptr, 0, total);
	return ptr;
}

void *__ec_realloc(void *ptr, size_t size, const char *file, unsigned int line)
{
	return ec_malloc_handler.realloc(ptr, size, file, line);
}

void ec_realloc_func(void *ptr, size_t size)
{
	ec_realloc(ptr, size);
}

char *__ec_strdup(const char *s, const char *file, unsigned int line)
{
	size_t sz = strlen(s) + 1;
	char *s2;

	s2 = __ec_malloc(sz, file, line);
	if (s2 == NULL)
		return NULL;

	memcpy(s2, s, sz);

	return s2;
}

char *__ec_strndup(const char *s, size_t n, const char *file, unsigned int line)
{
	size_t sz = strnlen(s, n);
	char *s2;

	s2 = __ec_malloc(sz + 1, file, line);
	if (s2 == NULL)
		return NULL;

	memcpy(s2, s, sz);
	s2[sz] = '\0';

	return s2;
}

static int ec_malloc_init_func(void)
{
	init_done = 1;
	return 0;
}

static struct ec_init ec_malloc_init = {
	.init = ec_malloc_init_func,
	.priority = 40,
};

EC_INIT_REGISTER(ec_malloc_init);
