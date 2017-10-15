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

#include "loader.h"

struct mtm_file_header {
	uint8 magic[3];		/* "MTM" */
	uint8 version;		/* MSN=major, LSN=minor */
	uint8 name[20];		/* ASCIIZ Module name */
	uint16 tracks;		/* Number of tracks saved */
	uint8 patterns;		/* Number of patterns saved */
	uint8 modlen;		/* Module length */
	uint16 extralen;	/* Length of the comment field */
	uint8 samples;		/* Number of samples */
	uint8 attr;		/* Always zero */
	uint8 rows;		/* Number rows per track */
	uint8 channels;		/* Number of tracks per pattern */
	uint8 pan[32];		/* Pan positions for each channel */
};

struct mtm_instrument_header {
	uint8 name[22];		/* Instrument name */
	uint32 length;		/* Instrument length in bytes */
	uint32 loop_start;	/* Sample loop start */
	uint32 loopend;		/* Sample loop end */
	uint8 finetune;		/* Finetune */
	uint8 volume;		/* Playback volume */
	uint8 attr;		/* &0x01: 16bit sample */
};

static int mtm_test(HIO_HANDLE *, char *, const int);
static int mtm_load(struct module_data *, HIO_HANDLE *, const int);

extern const struct format_loader libxmp_loader_mtm;
const struct format_loader libxmp_loader_mtm = {
	"Multitracker",
	mtm_test,
	mtm_load
};

static int mtm_test(HIO_HANDLE *f, char *t, const int start)
{
	uint8 buf[4];

	if (hio_read(buf, 1, 4, f) < 4)
		return -1;
	if (memcmp(buf, "MTM", 3))
		return -1;
	if (buf[3] != 0x10)
		return -1;

	libxmp_read_title(f, t, 20);

	return 0;
}

static int mtm_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j;
	struct mtm_file_header mfh;
	struct mtm_instrument_header mih;
	uint8 mt[192];

	LOAD_INIT();

	hio_read(&mfh.magic, 3, 1, f);	/* "MTM" */
	mfh.version = hio_read8(f);	/* MSN=major, LSN=minor */
	hio_read(&mfh.name, 20, 1, f);	/* ASCIIZ Module name */
	mfh.tracks = hio_read16l(f);	/* Number of tracks saved */
	mfh.patterns = hio_read8(f);	/* Number of patterns saved */
	mfh.modlen = hio_read8(f);	/* Module length */
	mfh.extralen = hio_read16l(f);	/* Length of the comment field */

	mfh.samples = hio_read8(f);	/* Number of samples */
	if (mfh.samples > 63) {
		return -1;
	}

	mfh.attr = hio_read8(f);	/* Always zero */

	mfh.rows = hio_read8(f);	/* Number rows per track */
	if (mfh.rows != 64)
		return -1;

	mfh.channels = hio_read8(f);	/* Number of tracks per pattern */
	if (mfh.channels > XMP_MAX_CHANNELS) {
		return -1;
	}

	hio_read(&mfh.pan, 32, 1, f);	/* Pan positions for each channel */

	if (hio_error(f)) {
		return -1;
	}

#if 0
	if (strncmp((char *)mfh.magic, "MTM", 3))
		return -1;
#endif

	mod->trk = mfh.tracks + 1;
	mod->pat = mfh.patterns + 1;
	mod->len = mfh.modlen + 1;
	mod->ins = mfh.samples;
	mod->smp = mod->ins;
	mod->chn = mfh.channels;
	mod->spd = 6;
	mod->bpm = 125;

	strncpy(mod->name, (char *)mfh.name, 20);
	libxmp_set_type(m, "MultiTracker %d.%02d MTM", MSN(mfh.version),
		 LSN(mfh.version));

	MODULE_INFO();

	if (libxmp_init_instrument(m) < 0)
		return -1;

	/* Read and convert instruments */
	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *xxi = &mod->xxi[i];
		struct xmp_sample *xxs = &mod->xxs[i];
		struct xmp_subinstrument *sub;

		if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
			return -1;

		sub = &xxi->sub[0];

		hio_read(&mih.name, 22, 1, f);	/* Instrument name */
		mih.length = hio_read32l(f);	/* Instrument length in bytes */

		if (mih.length > MAX_SAMPLE_SIZE)
			return -1;

		mih.loop_start = hio_read32l(f); /* Sample loop start */
		mih.loopend = hio_read32l(f);	/* Sample loop end */
		mih.finetune = hio_read8(f);	/* Finetune */
		mih.volume = hio_read8(f);	/* Playback volume */
		mih.attr = hio_read8(f);	/* &0x01: 16bit sample */

		xxs->len = mih.length;
		xxs->lps = mih.loop_start;
		xxs->lpe = mih.loopend;
		xxs->flg = xxs->lpe ? XMP_SAMPLE_LOOP : 0;	/* 1 == Forward loop */
		if (mfh.attr & 1) {
			xxs->flg |= XMP_SAMPLE_16BIT;
			xxs->len >>= 1;
			xxs->lps >>= 1;
			xxs->lpe >>= 1;
		}

		sub->vol = mih.volume;
		sub->fin = mih.finetune;
		sub->pan = 0x80;
		sub->sid = i;

		libxmp_instrument_name(mod, i, mih.name, 22);

		if (xxs->len > 0)
			mod->xxi[i].nsm = 1;

		D_(D_INFO "[%2X] %-22.22s %04x%c%04x %04x %c V%02x F%+03d\n", i,
		   xxi->name, xxs->len, xxs->flg & XMP_SAMPLE_16BIT ? '+' : ' ',
		   xxs->lps, xxs->lpe, xxs->flg & XMP_SAMPLE_LOOP ? 'L' : ' ',
		   sub->vol, sub->fin - 0x80);
	}

	hio_read(mod->xxo, 1, 128, f);

	if (libxmp_init_pattern(mod) < 0)
		return -1;

	D_(D_INFO "Stored tracks: %d", mod->trk - 1);

	for (i = 0; i < mod->trk; i++) {

		if (libxmp_alloc_track(mod, i, mfh.rows) < 0)
			return -1;

		if (i == 0)
			continue;

		if (hio_read(&mt, 3, 64, f) != 64)
			return -1;

		for (j = 0; j < 64; j++) {
			struct xmp_event *e = &mod->xxt[i]->event[j];
			uint8 *d = mt + j * 3;

			if ((e->note = d[0] >> 2)) {
				e->note += 37;
			}
			e->ins = ((d[0] & 0x3) << 4) + MSN(d[1]);
			e->fxt = LSN(d[1]);
			e->fxp = d[2];
			if (e->fxt > FX_SPEED) {
				e->fxt = e->fxp = 0;
			}

			/* Set pan effect translation */
			if (e->fxt == FX_EXTENDED && MSN(e->fxp) == 0x8) {
				e->fxt = FX_SETPAN;
				e->fxp <<= 4;
			}
		}
	}

	/* Read patterns */
	D_(D_INFO "Stored patterns: %d", mod->pat - 1);

	for (i = 0; i < mod->pat; i++) {
		if (libxmp_alloc_pattern(mod, i) < 0)
			return -1;

		mod->xxp[i]->rows = 64;
		for (j = 0; j < 32; j++) {
			int track = hio_read16l(f);

			if (track >= mod->trk) {
				track = 0;
			}

			if (j < mod->chn) {
				mod->xxp[i]->index[j] = track;
			}
		}
	}

	/* Comments */
	hio_seek(f, mfh.extralen, SEEK_CUR);

	/* Read samples */
	D_(D_INFO "Stored samples: %d", mod->smp);

	for (i = 0; i < mod->ins; i++) {
		if (libxmp_load_sample(m, f, SAMPLE_FLAG_UNS, &mod->xxs[i], NULL) < 0)
			return -1;
	}

	for (i = 0; i < mod->chn; i++)
		mod->xxc[i].pan = mfh.pan[i] << 4;

	return 0;
}
