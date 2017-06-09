#ifndef LIBXMP_MDATAIO_H
#define LIBXMP_MDATAIO_H

#include <stddef.h>
#include "common.h"

static inline ptrdiff_t CAN_READ(MFILE *m)
{
	if (m->size >= 0)
		return m->pos >= 0 ? m->size - m->pos : 0;

	return INT_MAX;
}

static inline uint8 mread8(MFILE *m)
{
	uint8 x = 0xff;
	mread(&x, 1, 1, m);
	return x;
}

static inline int8 mread8s(MFILE *m)
{
	return (int8)mgetc(m);
}

static inline uint16 mread16l(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16 n = readmem16l(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

static inline uint16 mread16b(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16 n = readmem16b(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

static inline uint32 mread24l(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 3) {
		uint32 n = readmem24l(m->start + m->pos);
		m->pos += 3;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

static inline uint32 mread24b(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 3) {
		uint32 n = readmem24b(m->start + m->pos);
		m->pos += 3;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

static inline uint32 mread32l(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32 n = readmem32l(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

static inline uint32 mread32b(MFILE *m)
{
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32 n = readmem32b(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		m->pos += can_read;
		return EOF;
	}
}

#endif
