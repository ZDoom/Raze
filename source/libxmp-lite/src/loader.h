#ifndef XMP_LOADER_H
#define XMP_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "effects.h"
#include "format.h"
#include "hio.h"

/* Sample flags */
#define SAMPLE_FLAG_DIFF	0x0001	/* Differential */
#define SAMPLE_FLAG_UNS		0x0002	/* Unsigned */
#define SAMPLE_FLAG_8BDIFF	0x0004
#define SAMPLE_FLAG_7BIT	0x0008
#define SAMPLE_FLAG_NOLOAD	0x0010	/* Get from buffer, don't load */
#define SAMPLE_FLAG_BIGEND	0x0040	/* Big-endian */
#define SAMPLE_FLAG_VIDC	0x0080	/* Archimedes VIDC logarithmic */
/*#define SAMPLE_FLAG_STEREO	0x0100	   Interleaved stereo sample */
#define SAMPLE_FLAG_FULLREP	0x0200	/* Play full sample before looping */
#define SAMPLE_FLAG_ADLIB	0x1000	/* Adlib synth instrument */
#define SAMPLE_FLAG_HSC		0x2000	/* HSC Adlib synth instrument */
#define SAMPLE_FLAG_ADPCM	0x4000	/* ADPCM4 encoded samples */

#define DEFPAN(x) (0x80 + ((x) - 0x80) * m->defpan / 100)

int	libxmp_init_instrument		(struct module_data *);
int	libxmp_alloc_subinstrument	(struct xmp_module *, int, int);
int	libxmp_init_pattern		(struct xmp_module *);
int	libxmp_alloc_pattern		(struct xmp_module *, int);
int	libxmp_alloc_track		(struct xmp_module *, int, int);
int	libxmp_alloc_tracks_in_pattern	(struct xmp_module *, int);
int	libxmp_alloc_pattern_tracks	(struct xmp_module *, int, int);
char	*libxmp_instrument_name		(struct xmp_module *, int, uint8 *, int);
struct xmp_sample* libxmp_realloc_samples(struct xmp_sample *, int *, int);

char	*libxmp_copy_adjust		(char *, uint8 *, int);
int	libxmp_test_name		(uint8 *, int);
void	libxmp_read_title		(HIO_HANDLE *, char *, int);
void	libxmp_set_xxh_defaults		(struct xmp_module *);
void	libxmp_decode_protracker_event	(struct xmp_event *, uint8 *);
void	libxmp_decode_noisetracker_event(struct xmp_event *, uint8 *);
void	libxmp_disable_continue_fx	(struct xmp_event *);
int	libxmp_check_filename_case	(char *, char *, char *, int);
void	libxmp_get_instrument_path	(struct module_data *, char *, int);
void	libxmp_set_type			(struct module_data *, const char *, ...);
int	libxmp_load_sample		(struct module_data *, HIO_HANDLE *, int,
					 struct xmp_sample *, const void *);

extern uint8		libxmp_ord_xlat[];
extern const int	libxmp_arch_vol_table[];

#define MAGIC4(a,b,c,d) \
    (((uint32)(a)<<24)|((uint32)(b)<<16)|((uint32)(c)<<8)|(d))

#define LOAD_INIT()

#define MODULE_INFO() do { \
    D_(D_WARN "Module title: \"%s\"", m->mod.name); \
    D_(D_WARN "Module type: %s", m->mod.type); \
} while (0)

#endif
