#ifndef XMP_HIO_H
#define XMP_HIO_H

#ifdef EDUKE32_DISABLED
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stddef.h>
#include "memio.h"

#define HIO_HANDLE_TYPE(x) ((x)->type)

typedef struct {
#ifdef EDUKE32_DISABLED
#define HIO_HANDLE_TYPE_FILE	0
#endif
#define HIO_HANDLE_TYPE_MEMORY	1
	int type;
	long size;
	union {
#ifdef EDUKE32_DISABLED
		FILE *file;
#endif
		MFILE *mem;
	} handle;
	int error;
} HIO_HANDLE;

int8	hio_read8s	(HIO_HANDLE *);
uint8	hio_read8	(HIO_HANDLE *);
uint16	hio_read16l	(HIO_HANDLE *);
uint16	hio_read16b	(HIO_HANDLE *);
uint32	hio_read24l	(HIO_HANDLE *);
uint32	hio_read24b	(HIO_HANDLE *);
uint32	hio_read32l	(HIO_HANDLE *);
uint32	hio_read32b	(HIO_HANDLE *);
size_t	hio_read	(void *, size_t, size_t, HIO_HANDLE *);	
int	hio_seek	(HIO_HANDLE *, long, int);
long	hio_tell	(HIO_HANDLE *);
int	hio_eof		(HIO_HANDLE *);
int	hio_error	(HIO_HANDLE *);
#ifdef EDUKE32_DISABLED
HIO_HANDLE *hio_open	(const void *, const char *);
#endif
HIO_HANDLE *hio_open_mem  (const void *, long);
#ifdef EDUKE32_DISABLED
HIO_HANDLE *hio_open_file (FILE *);
#endif
int	hio_close	(HIO_HANDLE *);
long	hio_size	(HIO_HANDLE *);

#endif
