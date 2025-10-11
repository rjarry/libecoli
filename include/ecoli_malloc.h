/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_alloc Allocation
 * @{
 *
 * @brief Helpers to allocate and free memory
 *
 * Interface to configure the allocator used by libecoli.
 * By default, the standard allocation functions from libc are used.
 */

#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

/**
 * Function type of malloc, passed to ec_malloc_register().
 *
 * The API is the same than malloc(), excepted the file and line
 * arguments.
 *
 * @param size
 *   The size of the memory area to allocate.
 * @param file
 *   The path to the file that invoked the malloc.
 * @param line
 *   The line in the file that invoked the malloc.
 * @return
 *   A pointer to the allocated memory area, or NULL on error (errno
 *   is set).
 */
typedef void *(*ec_malloc_t)(size_t size, const char *file, unsigned int line);

/**
 * Function type of free, passed to ec_malloc_register().
 *
 * The API is the same than free(), excepted the file and line
 * arguments.
 *
 * @param ptr
 *   The pointer to the memory area to be freed.
 * @param file
 *   The path to the file that invoked the malloc.
 * @param line
 *   The line in the file that invoked the malloc.
 */
typedef void (*ec_free_t)(void *ptr, const char *file, unsigned int line);

/**
 * Function type of realloc, passed to ec_malloc_register().
 *
 * The API is the same than realloc(), excepted the file and line
 * arguments.
 *
 * @param ptr
 *   The pointer to the memory area to be reallocated.
 * @param file
 *   The path to the file that invoked the malloc.
 * @param line
 *   The line in the file that invoked the malloc.
 * @return
 *   A pointer to the allocated memory area, or NULL on error (errno
 *   is set).
 */
typedef void *(*ec_realloc_t)(void *ptr, size_t size, const char *file,
	unsigned int line);

/**
 * Register allocation functions.
 *
 * This function can be use to register another allocator
 * to be used by libecoli. By default, ec_malloc(), ec_free() and
 * ec_realloc() use the standard libc allocator. Another handler
 * can be used for debug purposes or when running in a specific
 * environment.
 *
 * This function must be called before ec_init().
 *
 * @param usr_malloc
 *   A user-defined malloc function.
 * @param usr_free
 *   A user-defined free function.
 * @param usr_realloc
 *   A user-defined realloc function.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_malloc_register(ec_malloc_t usr_malloc, ec_free_t usr_free,
	ec_realloc_t usr_realloc);

struct ec_malloc_handler {
	ec_malloc_t malloc;
	ec_free_t free;
	ec_realloc_t realloc;
};

extern struct ec_malloc_handler ec_malloc_handler;

/**
 * Allocate a memory area.
 *
 * Like malloc(), ec_malloc() allocates size bytes and returns a pointer
 * to the allocated memory. The memory is not initialized. The memory is
 * freed with ec_free().
 *
 * @param size
 *   The size of the area to allocate in bytes.
 * @return
 *   The pointer to the allocated memory, or NULL on error (errno is set).
 */
#define ec_malloc(size) ({						\
	void *ret_;							\
	if (ec_malloc_handler.malloc == NULL)				\
		ret_ = malloc(size);					\
	else								\
		ret_ = __ec_malloc(size, __FILE__, __LINE__);		\
	ret_;								\
	})

/**
 * Ecoli malloc function.
 *
 * Use this function when the macro ec_malloc() cannot be used,
 * for instance when it is passed as a callback pointer.
 */
void *ec_malloc_func(size_t size);

/**
 * Free a memory area.
 *
 * Like free(), ec_free() frees the area pointed by ptr, which must have
 * been returned by a previous call to ec_malloc() or any other
 * allocation function of this file.
 *
 * @param ptr
 *   The pointer to the memory area.
 */
#define ec_free(ptr) ({							\
	if (ec_malloc_handler.free == NULL)				\
		free(ptr);						\
	else								\
		__ec_free(ptr, __FILE__, __LINE__);			\
	})

/**
 * Ecoli free function.
 *
 * Use this function when the macro ec_free() cannot be used,
 * for instance when it is passed as a callback pointer.
 */
void ec_free_func(void *ptr);

/**
 * Resize an allocated memory area.
 *
 * @param ptr
 *   The pointer to the previously allocated memory area, or NULL.
 * @param size
 *   The new size of the memory area.
 * @return
 *   A pointer to the newly allocated memory, or NULL if the request
 *   fails. In that case, the original area is left untouched.
 */
#define ec_realloc(ptr, size) ({					\
	void *ret_;							\
	if (ec_malloc_handler.realloc == NULL)				\
		ret_ = realloc(ptr, size);				\
	else								\
		ret_ = __ec_realloc(ptr, size, __FILE__, __LINE__);	\
	ret_;								\
	})

/**
 * Ecoli realloc function.
 *
 * Use this function when the macro ec_realloc() cannot be used,
 * for instance when it is passed as a callback pointer.
 */
void ec_realloc_func(void *ptr, size_t size);

/**
 * Allocate and initialize an array of elements.
 *
 * @param n
 *   The number of elements.
 * @param size
 *   The size of each element.
 * @return
 *   The pointer to the allocated memory, or NULL on error (errno is set).
 */
#define ec_calloc(n, size) ({						\
	void *ret_;							\
	if (ec_malloc_handler.malloc == NULL)				\
		ret_ = calloc(n, size);					\
	else								\
		ret_ = __ec_calloc(n, size, __FILE__, __LINE__);	\
	ret_;								\
	})

/**
 * Duplicate a string.
 *
 * Memory for the new string is obtained with ec_malloc(), and can be
 * freed with ec_free().
 *
 * @param s
 *   The string to be duplicated.
 * @return
 *   The pointer to the duplicated string, or NULL on error (errno is set).
 */
#define ec_strdup(s) ({							\
	void *ret_;							\
	if (ec_malloc_handler.malloc == NULL)				\
		ret_ = strdup(s);					\
	else								\
		ret_ = __ec_strdup(s, __FILE__, __LINE__);		\
	ret_;								\
	})

/**
 * Duplicate at most n bytes of a string.
 *
 * This function is similar to ec_strdup(), except that it copies at
 * most n bytes.  If s is longer than n, only n bytes are copied, and a
 * terminating null byte ('\0') is added.
 *
 * @param s
 *   The string to be duplicated.
 * @param n
 *   The maximum length of the new string.
 * @return
 *   The pointer to the duplicated string, or NULL on error (errno is set).
 */
#define ec_strndup(s, n) ({						\
	void *ret_;							\
	if (ec_malloc_handler.malloc == NULL)				\
		ret_ = strndup(s, n);					\
	else								\
		ret_ = __ec_strndup(s, n, __FILE__, __LINE__);		\
	ret_;								\
	})

/* internal */
void *__ec_malloc(size_t size, const char *file, unsigned int line);
void __ec_free(void *ptr, const char *file, unsigned int line);
void *__ec_calloc(size_t nmemb, size_t size, const char *file,
	unsigned int line);
void *__ec_realloc(void *ptr, size_t size, const char *file, unsigned int line);
char *__ec_strdup(const char *s, const char *file, unsigned int line);
char *__ec_strndup(const char *s, size_t n, const char *file,
	unsigned int line);

/** @} */
