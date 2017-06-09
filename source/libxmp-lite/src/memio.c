/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#ifndef LIBXMP_CORE_PLAYER
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "common.h"
#include "memio.h"

static inline ptrdiff_t CAN_READ(MFILE *m)
{
	if (m->size >= 0)
		return m->pos >= 0 ? m->size - m->pos : 0;

	return INT_MAX;
}


int mgetc(MFILE *m)
{
	if (CAN_READ(m) >= 1)
		return *(const uint8 *)(m->start + m->pos++);
	else
		return EOF;
}

size_t mread(void *buf, size_t size, size_t num, MFILE *m)
{
 	size_t should_read = size * num;
 	ptrdiff_t can_read = CAN_READ(m);

 	if (!size || !num || can_read <= 0) {
 		return 0;
	}

	if (should_read > can_read) {
 		should_read = can_read;
	}

	memcpy(buf, m->start + m->pos, should_read);
 	m->pos += should_read;

	return should_read / size;
}


int mseek(MFILE *m, long offset, int whence)
{
	switch (whence) {
	default:
	case SEEK_SET:
		if (m->size >= 0 && (offset > m->size || offset < 0))
			return -1;
		m->pos = offset;
		return 0;
	case SEEK_CUR:
		if (m->size >= 0 && (offset > CAN_READ(m) || offset < -m->pos))
			return -1;
		m->pos += offset;
		return 0;
	case SEEK_END:
		if (m->size < 0)
			return -1;
		m->pos = m->size + offset;
		return 0;
	}
}

long mtell(MFILE *m)
{
	return (long)m->pos;
}

int meof(MFILE *m)
{
	if (m->size <= 0)
		return 0;
	else
		return CAN_READ(m) <= 0;
}

MFILE *mopen(const void *ptr, long size)
{
	MFILE *m;

	m = (MFILE *)malloc(sizeof (MFILE));
	if (m == NULL)
		return NULL;
	
	m->start = (const unsigned char *)ptr;
	m->pos = 0;
	m->size = size;

	return m;
}

int mclose(MFILE *m)
{
	free(m);
	return 0;
}

#ifndef LIBXMP_CORE_PLAYER

int mstat(MFILE *m, struct stat *st)
{
	memset(st, 0, sizeof (struct stat));
	st->st_size = m->size;
	return 0;
}

#endif

