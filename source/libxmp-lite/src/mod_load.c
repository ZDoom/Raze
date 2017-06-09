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

/* This loader recognizes the following variants of the Protracker
 * module format:
 *
 * - Protracker M.K.
 * - Fasttracker ?CHN and ??CH
 */

#include <ctype.h>
#include <limits.h>
#include "loader.h"
#include "mod.h"

static int mod_test(HIO_HANDLE *, char *, const int);
static int mod_load(struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_mod = {
	"Protracker",
	mod_test,
	mod_load
};

static int mod_test(HIO_HANDLE *f, char *t, const int start)
{
	int i;
	char buf[4];

	hio_seek(f, start + 1080, SEEK_SET);
	if (hio_read(buf, 1, 4, f) < 4)
		return -1;

	if (!strncmp(buf + 2, "CH", 2) && isdigit((int)buf[0])
	    && isdigit((int)buf[1])) {
		i = (buf[0] - '0') * 10 + buf[1] - '0';
		if (i > 0 && i <= 32) {
			goto found;
		}
	}

	if (!strncmp(buf + 1, "CHN", 3) && isdigit((int)*buf)) {
		if (*buf >= '0' && *buf <= '9') {
			goto found;
		}
	}

	if (memcmp(buf, "M.K.", 4))
		return -1;

found:
	hio_seek(f, start + 0, SEEK_SET);
	libxmp_read_title(f, t, 20);

	return 0;
}

static int mod_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j;
	struct xmp_event *event;
	struct mod_header mh;
	uint8 mod_event[4];
	char magic[8];
	int ptkloop = 0;	/* Protracker loop */

	LOAD_INIT();

	mod->ins = 31;
	mod->smp = mod->ins;
	mod->chn = 0;

	m->quirk |= QUIRK_PROTRACK;
	m->period_type = PERIOD_MODRNG;

	hio_read(&mh.name, 20, 1, f);
	for (i = 0; i < 31; i++) {
		hio_read(&mh.ins[i].name, 22, 1, f);	/* Instrument name */
		mh.ins[i].size = hio_read16b(f);	/* Length in 16-bit words */
		mh.ins[i].finetune = hio_read8(f);	/* Finetune (signed nibble) */
		mh.ins[i].volume = hio_read8(f);	/* Linear playback volume */
		mh.ins[i].loop_start = hio_read16b(f);	/* Loop start in 16-bit words */
		mh.ins[i].loop_size = hio_read16b(f);	/* Loop size in 16-bit words */
	}
	mh.len = hio_read8(f);
	mh.restart = hio_read8(f);
	hio_read(&mh.order, 128, 1, f);
	memset(magic, 0, 8);
	hio_read(magic, 4, 1, f);

	if (!memcmp(magic, "M.K.", 4)) {
		mod->chn = 4;
	} else if (!strncmp(magic + 2, "CH", 2) &&
		   isdigit((int)magic[0]) && isdigit((int)magic[1])) {
		mod->chn = (*magic - '0') * 10 + magic[1] - '0';
	} else if (!strncmp(magic + 1, "CHN", 3) && isdigit((int)*magic)) {
		mod->chn = *magic - '0';
	} else {
		return -1;
	}

	strncpy(mod->name, (char *)mh.name, 20);

	mod->len = mh.len;
	/* mod->rst = mh.restart; */

	if (mod->rst >= mod->len)
		mod->rst = 0;
	memcpy(mod->xxo, mh.order, 128);

	for (i = 0; i < 128; i++) {
		/* This fixes dragnet.mod (garbage in the order list) */
		if (mod->xxo[i] > 0x7f)
			break;
		if (mod->xxo[i] > mod->pat)
			mod->pat = mod->xxo[i];
	}
	mod->pat++;

	if (libxmp_init_instrument(m) < 0)
		return -1;

	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *xxi;
		struct xmp_subinstrument *sub;
		struct xmp_sample *xxs;

		if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
			return -1;

		xxi = &mod->xxi[i];
		sub = &xxi->sub[0];
		xxs = &mod->xxs[i];

		xxs->len = 2 * mh.ins[i].size;
		xxs->lps = 2 * mh.ins[i].loop_start;
		xxs->lpe = xxs->lps + 2 * mh.ins[i].loop_size;
		if (xxs->lpe > xxs->len) {
			xxs->lpe = xxs->len;
		}
		xxs->flg = (mh.ins[i].loop_size > 1 && xxs->lpe >= 4) ?
		    XMP_SAMPLE_LOOP : 0;
		sub->fin = (int8) (mh.ins[i].finetune << 4);
		sub->vol = mh.ins[i].volume;
		sub->pan = 0x80;
		sub->sid = i;
		libxmp_instrument_name(mod, i, mh.ins[i].name, 22);

		if (xxs->len > 0) {
			xxi->nsm = 1;
		}
	}

	mod->trk = mod->chn * mod->pat;

	libxmp_set_type(m, mod->chn == 4 ? "Protracker" : "Fasttracker");

	MODULE_INFO();

	for (i = 0; i < mod->ins; i++) {
		D_(D_INFO "[%2X] %-22.22s %04x %04x %04x %c V%02x %+d %c\n",
		   i, mod->xxi[i].name,
		   mod->xxs[i].len, mod->xxs[i].lps, mod->xxs[i].lpe,
		   (mh.ins[i].loop_size > 1 && mod->xxs[i].lpe > 8) ?
		   'L' : ' ', mod->xxi[i].sub[0].vol,
		   mod->xxi[i].sub[0].fin >> 4,
		   ptkloop && mod->xxs[i].lps == 0 && mh.ins[i].loop_size > 1 &&
		   mod->xxs[i].len > mod->xxs[i].lpe ? '!' : ' ');
	}

	if (libxmp_init_pattern(mod) < 0)
		return -1;

	/* Load and convert patterns */
	D_(D_INFO "Stored patterns: %d", mod->pat);

	for (i = 0; i < mod->pat; i++) {
		if (libxmp_alloc_pattern_tracks(mod, i, 64) < 0)
			return -1;

		for (j = 0; j < (64 * mod->chn); j++) {
			event = &EVENT(i, j % mod->chn, j / mod->chn);
			hio_read(mod_event, 1, 4, f);
			libxmp_decode_protracker_event(event, mod_event);
		}
	}

	/* Load samples */

	D_(D_INFO "Stored samples: %d", mod->smp);

	for (i = 0; i < mod->smp; i++) {
		int flags;

		if (!mod->xxs[i].len)
			continue;

		flags = ptkloop ? SAMPLE_FLAG_FULLREP : 0;

		if (libxmp_load_sample(m, f, flags, &mod->xxs[i], NULL) < 0)
			return -1;
	}

	if (mod->chn > 4) {
		m->quirk &= ~QUIRK_PROTRACK;
		m->quirk |= QUIRKS_FT2 | QUIRK_FTMOD;
		m->read_event_type = READ_EVENT_FT2;
		m->period_type = PERIOD_AMIGA;
	}

	return 0;
}
