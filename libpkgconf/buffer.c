/*
 * buffer.c
 * dynamically-managed buffers
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2024 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

/*
 * !doc
 *
 * libpkgconf `buffer` module
 * ==========================
 *
 * The libpkgconf `buffer` module contains the functions related to managing
 * dynamically-allocated buffers.
 */

static inline bool
target_allocation_size(size_t target_size, size_t *allocation_size)
{
	size_t growth = 128 - (target_size % 128);

	if (target_size > SIZE_MAX - growth)
		return false;

	*allocation_size = target_size + growth;
	return true;
}

static inline bool
buffer_reserve(pkgconf_buffer_t *buffer, size_t content_len)
{
	size_t new_cap, old_cap;
	char *newbase;

	if (!target_allocation_size(content_len, &new_cap))
		return false;

	if (buffer->base != NULL && target_allocation_size(pkgconf_buffer_len(buffer), &old_cap) && old_cap == new_cap)
		return true;

	newbase = realloc(buffer->base, new_cap);
	if (newbase == NULL)
		return false;

	buffer->base = newbase;
	return true;
}

static bool
buffer_storage_overlaps(const pkgconf_buffer_t *buffer, const void *source, size_t source_size)
{
	if (buffer->base == NULL || source == NULL || source_size == 0)
		return false;

	size_t buffer_size = pkgconf_buffer_len(buffer) + 1;
	uintptr_t buffer_start = (uintptr_t) buffer->base;
	uintptr_t source_start = (uintptr_t) source;

	if (buffer_start > UINTPTR_MAX - (buffer_size - 1) ||
		source_start > UINTPTR_MAX - (source_size - 1))
		return true;

	uintptr_t buffer_end = buffer_start + buffer_size - 1;
	uintptr_t source_end = source_start + source_size - 1;

	return buffer_start <= source_end && source_start <= buffer_end;
}

#if 0
static void
buffer_debug(pkgconf_buffer_t *buffer)
{
	for (char *c = buffer->base; c <= buffer->end; c++)
	{
		fprintf(stderr, "%02x ", (unsigned char) *c);
	}

	pkgconf_output_file_fmt(stderr, "\n");
}
#endif

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_append(pkgconf_buffer_t *buffer, const char *text)
 *
 *    Append a null-terminated string to the buffer, reallocating as necessary.
 *    The storage occupied by *text* must not overlap the buffer.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to append to.
 *    :param char *text: The null-terminated string to append.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_append(pkgconf_buffer_t *buffer, const char *text)
{
	size_t len = pkgconf_buffer_len(buffer);
	size_t text_len = strlen(text);

	if (text_len == SIZE_MAX)
		return false;

	if (buffer_storage_overlaps(buffer, text, text_len + 1))
		return false;

	if (len > SIZE_MAX - text_len)
		return false;

	size_t new_len = len + text_len;
	if (!buffer_reserve(buffer, new_len))
		return false;

	memcpy(buffer->base + len, text, text_len);

	buffer->end = buffer->base + new_len;
	*buffer->end = '\0';

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_append_slice(pkgconf_buffer_t *buf, const char *p, size_t n)
 *
 *    Append a slice of *n* bytes to the buffer. Does nothing if *n* is zero.
 *    The source slice must not overlap the buffer.
 *
 *    :param pkgconf_buffer_t *buf: The buffer to append to.
 *    :param char *p: Pointer to the byte sequence to append.
 *    :param size_t n: Number of bytes to append.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_append_slice(pkgconf_buffer_t *buf, const char *p, size_t n)
{
	if (n == 0)
		return true;

	if (buffer_storage_overlaps(buf, p, n))
		return false;

	for (size_t i = 0; i < n; i++)
	{
		if (!pkgconf_buffer_push_byte(buf, p[i]))
			return false;
	}

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_append_vfmt(pkgconf_buffer_t *buffer, const char *fmt, va_list src_va)
 *
 *    Append a formatted string to the buffer using a :code:`va_list`.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to append to.
 *    :param char *fmt: A printf-style format string.
 *    :param va_list src_va: The variadic argument list for the format string.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_append_vfmt(pkgconf_buffer_t *buffer, const char *fmt, va_list src_va)
{
	va_list va;
	char *buf;
	size_t needed;
	int formatted_len;

	va_copy(va, src_va);
	formatted_len = vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	if (formatted_len < 0)
		return false;

	needed = (size_t) formatted_len + 1;
	buf = malloc(needed);
	if (buf == NULL)
		return false;

	va_copy(va, src_va);
	vsnprintf(buf, needed, fmt, va);
	va_end(va);

	bool ret = pkgconf_buffer_append(buffer, buf);

	free(buf);

	return ret;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_append_fmt(pkgconf_buffer_t *buffer, const char *fmt, ...)
 *
 *    Append a formatted string to the buffer using variadic arguments.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to append to.
 *    :param char *fmt: A printf-style format string.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_append_fmt(pkgconf_buffer_t *buffer, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	bool ret = pkgconf_buffer_append_vfmt(buffer, fmt, va);
	va_end(va);

	return ret;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_prepend(pkgconf_buffer_t *buffer, const char *text)
 *
 *    Prepend a null-terminated string to the beginning of the buffer.
 *    If *text* is NULL, the buffer contents are unchanged.
 *    The storage occupied by *text* must not overlap the buffer.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to prepend to.
 *    :param char *text: The null-terminated string to prepend, or NULL.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_prepend(pkgconf_buffer_t *buffer, const char *text)
{
	pkgconf_buffer_t tmpbuf = PKGCONF_BUFFER_INITIALIZER;
	bool ret = true;

	if (text != NULL)
	{
		size_t text_len = strlen(text);

		if (text_len == SIZE_MAX ||
			buffer_storage_overlaps(buffer, text, text_len + 1))
			return false;
	}

	if (text != NULL && !pkgconf_buffer_append(&tmpbuf, text))
		return false;

	if (!pkgconf_buffer_append(&tmpbuf, pkgconf_buffer_str_or_empty(buffer)))
	{
		pkgconf_buffer_finalize(&tmpbuf);
		return false;
	}

	if (pkgconf_buffer_len(&tmpbuf))
		ret = pkgconf_buffer_copy(&tmpbuf, buffer);

	pkgconf_buffer_finalize(&tmpbuf);

	return ret;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_push_byte(pkgconf_buffer_t *buffer, char byte)
 *
 *    Append a single byte to the buffer, reallocating as necessary.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to append to.
 *    :param char byte: The byte to append.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_push_byte(pkgconf_buffer_t *buffer, char byte)
{
	size_t len = pkgconf_buffer_len(buffer);

	if (len == SIZE_MAX)
		return false;

	size_t new_len = len + 1;
	if (!buffer_reserve(buffer, new_len))
		return false;

	buffer->base[len] = byte;

	buffer->end = buffer->base + new_len;
	*buffer->end = '\0';

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_trim_byte(pkgconf_buffer_t *buffer)
 *
 *    Remove the last byte from the buffer. The buffer must be non-empty.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to trim.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_trim_byte(pkgconf_buffer_t *buffer)
{
	size_t len = pkgconf_buffer_len(buffer);
	if (len == 0)
		return false;

	size_t new_len = len - 1;
	if (!buffer_reserve(buffer, new_len))
		return false;

	buffer->end = buffer->base + new_len;
	*(buffer->end) = '\0';

	return true;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_buffer_finalize(pkgconf_buffer_t *buffer)
 *
 *    Free all memory owned by the buffer and reset it to an empty state.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to finalize.
 *    :return: nothing
 */
void
pkgconf_buffer_finalize(pkgconf_buffer_t *buffer)
{
	free(buffer->base);
	buffer->base = buffer->end = NULL;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_fputs(pkgconf_buffer_t *buffer, FILE *out)
 *
 *    Write the buffer contents followed by a newline to a file stream.
 *    If the buffer is empty, only a newline is written.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to write.
 *    :param FILE *out: The output file stream.
 *    :return: :code:`true` on success, :code:`false` on I/O error.
 *             :code:`errno` will be set by fputs/fputc.
 */
bool
pkgconf_buffer_fputs(pkgconf_buffer_t *buffer, FILE *out)
{
	if (pkgconf_buffer_len(buffer) != 0)
	{
		if (fputs(pkgconf_buffer_str(buffer), out) == EOF)
			return false;
	}

	if (fputc('\n', out) == EOF)
		return false;

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_vjoin(pkgconf_buffer_t *buffer, char delim, va_list src_va)
 *
 *    Join a NULL-terminated list of strings into the buffer, separated by *delim*.
 *    Uses a :code:`va_list` for the string arguments.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to join into.
 *    :param char delim: The delimiter byte inserted between each argument.
 *    :param va_list src_va: The variadic argument list of :code:`const char *` strings, terminated by NULL.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_vjoin(pkgconf_buffer_t *buffer, char delim, va_list src_va)
{
	va_list va;
	const char *arg;

	va_copy(va, src_va);

	while ((arg = va_arg(va, const char *)) != NULL)
	{
		if (pkgconf_buffer_str(buffer) != NULL)
		{
			if (!pkgconf_buffer_push_byte(buffer, delim))
			{
				va_end(va);
				return false;
			}
		}

		if (!pkgconf_buffer_append(buffer, arg))
		{
			va_end(va);
			return false;
		}
	}

	va_end(va);

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_join(pkgconf_buffer_t *buffer, int delim, ...)
 *
 *    Join a NULL-terminated list of strings into the buffer, separated by *delim*.
 *    The *delim* parameter is typed as :code:`int` due to C variadic promotion rules.
 *
 *    :param pkgconf_buffer_t *buffer: The buffer to join into.
 *    :param int delim: The delimiter byte inserted between each argument (cast to :code:`char` internally).
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_join(pkgconf_buffer_t *buffer, int delim, ...)
{
	va_list va;

	va_start(va, delim);
	bool ret = pkgconf_buffer_vjoin(buffer, (char)delim, va);
	va_end(va);

	return ret;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_has_prefix(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *prefix)
 *
 *    Test whether the buffer begins with the contents of *prefix*.
 *
 *    :param pkgconf_buffer_t *haystack: The buffer to search in.
 *    :param pkgconf_buffer_t *prefix: The prefix to test for.
 *    :return: :code:`true` if *haystack* starts with *prefix*, :code:`false` otherwise.
 */
bool
pkgconf_buffer_has_prefix(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *prefix)
{
	const char *haystack_str = pkgconf_buffer_str_or_empty(haystack);
	const char *prefix_str = pkgconf_buffer_str_or_empty(prefix);

	return strncmp(haystack_str, prefix_str, strlen(prefix_str)) == 0;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_contains(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *needle)
 *
 *    Test whether the buffer contains the contents of *needle* as a substring.
 *
 *    :param pkgconf_buffer_t *haystack: The buffer to search in.
 *    :param pkgconf_buffer_t *needle: The substring to search for.
 *    :return: :code:`true` if *needle* is found, :code:`false` otherwise.
 */
bool
pkgconf_buffer_contains(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *needle)
{
	const char *haystack_str = pkgconf_buffer_str_or_empty(haystack);
	const char *needle_str = pkgconf_buffer_str_or_empty(needle);

	return strstr(haystack_str, needle_str) != NULL;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_contains_byte(const pkgconf_buffer_t *haystack, char needle)
 *
 *    Test whether the buffer contains a given byte.
 *
 *    :param pkgconf_buffer_t *haystack: The buffer to search in.
 *    :param char needle: The byte to search for.
 *    :return: :code:`true` if *needle* is found, :code:`false` otherwise.
 */

bool
pkgconf_buffer_contains_byte(const pkgconf_buffer_t *haystack, char needle)
{
	const char *haystack_str = pkgconf_buffer_str_or_empty(haystack);
	return strchr(haystack_str, needle) != NULL;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_match(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *needle)
 *
 *    Test whether two buffers have identical contents.
 *
 *    :param pkgconf_buffer_t *haystack: The first buffer.
 *    :param pkgconf_buffer_t *needle: The second buffer.
 *    :return: :code:`true` if the buffers have the same length and contents, :code:`false` otherwise.
 */
bool
pkgconf_buffer_match(const pkgconf_buffer_t *haystack, const pkgconf_buffer_t *needle)
{
	const char *haystack_str = pkgconf_buffer_str_or_empty(haystack);
	const char *needle_str = pkgconf_buffer_str_or_empty(needle);

	if (pkgconf_buffer_len(haystack) != pkgconf_buffer_len(needle))
		return false;

	return memcmp(haystack_str, needle_str, pkgconf_buffer_len(haystack)) == 0;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_subst(pkgconf_buffer_t *dest, const pkgconf_buffer_t *src, const char *pattern, const char *value)
 *
 *    Copy *src* into *dest*, replacing all occurrences of *pattern* with *value*.
 *    If *pattern* is empty, *src* is appended to *dest* unmodified.
 *    Neither *src*, *pattern* nor *value* may overlap *dest*.
 *
 *    :param pkgconf_buffer_t *dest: The destination buffer.
 *    :param pkgconf_buffer_t *src: The source buffer.
 *    :param char *pattern: The pattern string to search for.
 *    :param char *value: The replacement string.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_subst(pkgconf_buffer_t *dest, const pkgconf_buffer_t *src, const char *pattern, const char *value)
{
	const char *iter = src->base;

	if (dest == src ||
		buffer_storage_overlaps(dest, src->base, pkgconf_buffer_len(src) + 1))
		return false;

	if (pattern == NULL)
		pattern = "";

	if (value == NULL)
		value = "";

	size_t pattern_len = strlen(pattern);
	if (buffer_storage_overlaps(dest, pattern, pattern_len + 1) ||
		buffer_storage_overlaps(dest, value, strlen(value) + 1))
		return false;

	if (!pkgconf_buffer_len(src))
		return true;

	if (!pattern_len)
		return pkgconf_buffer_append(dest, pkgconf_buffer_str(src));

	while (iter < src->end)
	{
		if ((size_t)(src->end - iter) >= pattern_len && !memcmp(iter, pattern, pattern_len))
		{
			if (!pkgconf_buffer_append(dest, value))
				return false;

			iter += pattern_len;
		}
		else
		{
			if (!pkgconf_buffer_push_byte(dest, *iter++))
				return false;
		}
	}

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_buffer_escape(pkgconf_buffer_t *dest, const pkgconf_buffer_t *src, const pkgconf_span_t *spans, size_t nspans)
 *
 *    Copy *src* into *dest*, inserting a backslash before any byte that falls
 *    within the provided character spans.
 *    The source and destination buffers must not overlap.
 *
 *    :param pkgconf_buffer_t *dest: The destination buffer.
 *    :param pkgconf_buffer_t *src: The source buffer.
 *    :param pkgconf_span_t *spans: Array of character spans to escape.
 *    :param size_t nspans: Number of entries in the *spans* array.
 *    :return: :code:`true` on success, :code:`false` on allocation failure.
 */
bool
pkgconf_buffer_escape(pkgconf_buffer_t *dest, const pkgconf_buffer_t *src, const pkgconf_span_t *spans, size_t nspans)
{
	const char *p = pkgconf_buffer_str(src);

	if (dest == src ||
		buffer_storage_overlaps(dest, src->base, pkgconf_buffer_len(src) + 1))
		return false;

	if (!pkgconf_buffer_len(src))
		return true;

	for (; *p; p++)
	{
		if (pkgconf_span_contains((unsigned char) *p, spans, nspans))
		{
			if (!pkgconf_buffer_push_byte(dest, '\\'))
				return false;
		}

		if (!pkgconf_buffer_push_byte(dest, *p))
			return false;
	}

	return true;
}
