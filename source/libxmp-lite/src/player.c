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

/*
 * Sat, 18 Apr 1998 20:23:07 +0200  Frederic Bujon <lvdl@bigfoot.com>
 * Pan effect bug fixed: In Fastracker II the track panning effect erases
 * the instrument panning effect, and the same should happen in xmp.
 */

/*
 * Fri, 26 Jun 1998 13:29:25 -0400 (EDT)
 * Reported by Jared Spiegel <spieg@phair.csh.rit.edu>
 * when the volume envelope is not enabled (disabled) on a sample, and a
 * notoff is delivered to ft2 (via either a noteoff in the note column or
 * command Kxx [where xx is # of ticks into row to give a noteoff to the
 * sample]), ft2 will set the volume of playback of the sample to 00h.
 *
 * Claudio's fix: implementing effect K
 */

#include <stdlib.h>
#include <string.h>
#include "virtual.h"
#include "period.h"
#include "effects.h"
#include "player.h"
#include "mixer.h"
#ifndef LIBXMP_CORE_PLAYER
#include "extras.h"
#endif

/* Values for multi-retrig */
static const struct retrig_control rval[] = {
	{   0,  1,  1 }, {  -1,  1,  1 }, {  -2,  1,  1 }, {  -4,  1,  1 },
	{  -8,  1,  1 }, { -16,  1,  1 }, {   0,  2,  3 }, {   0,  1,  2 },
	{   0,  1,  1 }, {   1,  1,  1 }, {   2,  1,  1 }, {   4,  1,  1 },
	{   8,  1,  1 }, {  16,  1,  1 }, {   0,  3,  2 }, {   0,  2,  1 },
	{   0,  0,  1 }		/* Note cut */
	
};


/*
 * "Anyway I think this is the most brilliant piece of crap we
 *  have managed to put up!"
 *			  -- Ice of FC about "Mental Surgery"
 */


/* Envelope */

static int check_envelope_end(struct xmp_envelope *env, int x)
{
	int16 *data = env->data;
	int index;

	if (~env->flg & XMP_ENVELOPE_ON || env->npt <= 0)
		return 0;

	index = (env->npt - 1) * 2;

	/* last node */
	if (x >= data[index] || index == 0) { 
		if (~env->flg & XMP_ENVELOPE_LOOP) {
			return 1;
		}
	}

	return 0;
}

static int get_envelope(struct xmp_envelope *env, int x, int def)
{
	int x1, x2, y1, y2;
	int16 *data = env->data;
	int index;

	if (x < 0 || ~env->flg & XMP_ENVELOPE_ON || env->npt <= 0)
		return def;

	index = (env->npt - 1) * 2;

	x1 = data[index];		/* last node */
	if (x >= x1 || index == 0) { 
		return data[index + 1];
	}

	do {
		index -= 2;
		x1 = data[index];
	} while (index > 0 && x1 > x);

	/* interpolate */
	y1 = data[index + 1];
	x2 = data[index + 2];
	y2 = data[index + 3];

	return x2 == x1 ? y2 : ((y2 - y1) * (x - x1) / (x2 - x1)) + y1;
}

static int update_envelope_xm(struct xmp_envelope *env, int x, int release)
{
	int16 *data = env->data;
	int has_loop, has_sus;
	int lpe, lps, sus;

	has_loop = env->flg & XMP_ENVELOPE_LOOP;
	has_sus = env->flg & XMP_ENVELOPE_SUS;

	lps = env->lps << 1;
	lpe = env->lpe << 1;
	sus = env->sus << 1;

	/* FT2 and IT envelopes behave in a different way regarding loops,
	 * sustain and release. When the sustain point is at the end of the
	 * envelope loop end and the key is released, FT2 escapes the loop
	 * while IT runs another iteration. (See EnvLoops.xm in the OpenMPT
	 * test cases.)
	 */
	if (has_loop && has_sus && sus == lpe) {
		if (!release)
			has_sus = 0;
	}

	/* If the envelope point is set to somewhere after the sustain point
	 * or sustain loop, enable release to prevent the envelope point to
	 * return to the sustain point or loop start. (See Filip Skutela's
	 * farewell_tear.xm.)
	 */
	if (has_loop && x > data[lpe] + 1) {
		release = 1;
	} else if (has_sus && x > data[sus] + 1) {
		release = 1;
	}

	/* If enabled, stay at the sustain point */
	if (has_sus && !release) {
		if (x >= data[sus]) {
			x = data[sus];
		}
	}

	/* Envelope loops */
	if (has_loop && x >= data[lpe]) {
		if (!(release && has_sus && sus == lpe))
			x = data[lps];
	}

	return x;
}

#ifndef LIBXMP_CORE_DISABLE_IT

static int update_envelope_it(struct xmp_envelope *env, int x, int release, int key_off)
{
	int16 *data = env->data;
	int has_loop, has_sus;
	int lpe, lps, sus, sue;

	has_loop = env->flg & XMP_ENVELOPE_LOOP;
	has_sus = env->flg & XMP_ENVELOPE_SUS;

	lps = env->lps << 1;
	lpe = env->lpe << 1;
	sus = env->sus << 1;
	sue = env->sue << 1;

	/* Release at the end of a sustain loop, run another loop */
	if (has_sus && key_off && x == data[sue] + 1) {
		x = data[sus];
	} else
	/* If enabled, stay in the sustain loop */
	if (has_sus && !release) {
		if (x == data[sue] + 1) {
			x = data[sus];
		}
	} else
	/* Finally, execute the envelope loop */
	if (has_loop) {
		if (x > data[lpe]) {
			x = data[lps];
		}
	}

	return x;
}

#endif

static int update_envelope(struct xmp_envelope *env, int x, int release, int key_off, int it_env)
{
	if (x < 0xffff)	{	/* increment tick */
		x++;
	}

	if (x < 0) {
		return -1;
	}

	if (~env->flg & XMP_ENVELOPE_ON || env->npt <= 0) {
		return x;
	}

#ifndef LIBXMP_CORE_DISABLE_IT
	return it_env ?
		update_envelope_it(env, x, release, key_off) :
		update_envelope_xm(env, x, release);
#else
	return update_envelope_xm(env, x, release);
#endif
}


/* Returns: 0 if do nothing, <0 to reset channel, >0 if has fade */
static int check_envelope_fade(struct xmp_envelope *env, int x)
{
	int16 *data = env->data;
	int index;

	if (~env->flg & XMP_ENVELOPE_ON)
		return 0;

	index = (env->npt - 1) * 2;		/* last node */
	if (x > data[index]) {
		if (data[index + 1] == 0)
			return -1;
		else
			return 1;
	}

	return 0;
}


#ifndef LIBXMP_CORE_PLAYER

/* From http://www.un4seen.com/forum/?topic=7554.0
 *
 * "Invert loop" effect replaces (!) sample data bytes within loop with their
 * bitwise complement (NOT). The parameter sets speed of altering the samples.
 * This effectively trashes the sample data. Because of that this effect was
 * supposed to be removed in the very next ProTracker versions, but it was
 * never removed.
 *
 * Prior to [Protracker 1.1A] this effect is called "Funk Repeat" and it moves
 * loop of the instrument (just the loop information - sample data is not
 * altered). The parameter is the speed of moving the loop.
 */

static const int invloop_table[] = {
	0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128
};

static void update_invloop(struct module_data *m, struct channel_data *xc)
{
	struct xmp_sample *xxs = &m->mod.xxs[xc->smp];
	int len;

	xc->invloop.count += invloop_table[xc->invloop.speed];

	if ((xxs->flg & XMP_SAMPLE_LOOP) && xc->invloop.count >= 128) {
		xc->invloop.count = 0;
		len = xxs->lpe - xxs->lps;	

		if (++xc->invloop.pos > len) {
			xc->invloop.pos = 0;
		}

		if (~xxs->flg & XMP_SAMPLE_16BIT) {
			xxs->data[xxs->lps + xc->invloop.pos] ^= 0xff;
		}
	}
}

#endif

/*
 * From OpenMPT Arpeggio.xm test:
 *
 * "[FT2] Arpeggio behavior is very weird with more than 16 ticks per row. This
 *  comes from the fact that Fasttracker 2 uses a LUT for computing the arpeggio
 *  note (instead of doing something like tick%3 or similar). The LUT only has
 *  16 entries, so when there are more than 16 ticks, it reads beyond array
 *  boundaries. The vibrato table happens to be stored right after arpeggio
 *  table. The tables look like this in memory:
 *
 *    ArpTab: 0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0
 *    VibTab: 0,24,49,74,97,120,141,161,180,197,...
 *
 *  All values except for the first in the vibrato table are greater than 1, so
 *  they trigger the third arpeggio note. Keep in mind that Fasttracker 2 counts
 *  downwards, so the table has to be read from back to front, i.e. at 16 ticks
 *  per row, the 16th entry in the LUT is the first to be read. This is also the
 *  reason why Arpeggio is played 'backwards' in Fasttracker 2."
 */
static int ft2_arpeggio(struct context_data *ctx, struct channel_data *xc)
{
	struct player_data *p = &ctx->p;
	int i;

	if (xc->arpeggio.val[1] == 0 && xc->arpeggio.val[2] == 0) {
		return 0;
	}

	if (p->frame == 0) {
		return 0;
	}

	i = p->speed - (p->frame % p->speed);

	if (i == 16) {
		return 0;
	} else if (i > 16) {
		return xc->arpeggio.val[2];
	}

	return xc->arpeggio.val[i % 3];
}

static int is_first_frame(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;

	switch (m->read_event_type) {
#ifndef LIBXMP_CORE_DISABLE_IT
	case READ_EVENT_IT:
		/* fall through */
#endif
	case READ_EVENT_ST3:
		return p->frame % p->speed == 0;
	default:
		return p->frame == 0;
	}
}

static void reset_channels(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct smix_data *smix = &ctx->smix;
	struct channel_data *xc;
	int i;

#ifndef LIBXMP_CORE_PLAYER
	for (i = 0; i < p->virt.virt_channels; i++) {
		void *extra;

		xc = &p->xc_data[i];
		extra = xc->extra;
		memset(xc, 0, sizeof (struct channel_data));
		xc->extra = extra;
		libxmp_reset_channel_extras(ctx, xc);
		xc->ins = -1;
		xc->old_ins = 1;	/* raw value */
		xc->key = -1;
		xc->volume = m->volbase;
	}
#else
	for (i = 0; i < p->virt.virt_channels; i++) {
		xc = &p->xc_data[i];
		memset(xc, 0, sizeof (struct channel_data));
		xc->ins = -1;
		xc->old_ins = 1;	/* raw value */
		xc->key = -1;
		xc->volume = m->volbase;
	}
#endif

	for (i = 0; i < p->virt.num_tracks; i++) {
		xc = &p->xc_data[i];

		if (i >= mod->chn && i < mod->chn + smix->chn) {
			xc->mastervol = 0x40;
			xc->pan.val = 0x80;
		} else {
			xc->mastervol = mod->xxc[i].vol;
			xc->pan.val = mod->xxc[i].pan;
		}
		
#ifndef LIBXMP_CORE_DISABLE_IT
		xc->filter.cutoff = 0xff;

		/* Amiga split channel */
		if (mod->xxc[i].flg & XMP_CHANNEL_SPLIT) {
			int j;

			xc->split = ((mod->xxc[i].flg & 0x30) >> 4) + 1;
			/* Connect split channel pairs */
			for (j = 0; j < i; j++) {
				if (mod->xxc[j].flg & XMP_CHANNEL_SPLIT) {
					if (p->xc_data[j].split == xc->split) {
						p->xc_data[j].pair = i;
						xc->pair = j;
					}
				}
			}
		} else {
			xc->split = 0;
		}
#endif

		/* Surround channel */
		if (mod->xxc[i].flg & XMP_CHANNEL_SURROUND) {
			xc->pan.surround = 1;
		}
	}
}

static int check_delay(struct context_data *ctx, struct xmp_event *e, int chn)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];
	struct module_data *m = &ctx->m;

	/* Tempo affects delay and must be computed first */
	if ((e->fxt == FX_SPEED && e->fxp < 0x20) || e->fxt == FX_S3M_SPEED) {
		if (e->fxp) {
			p->speed = e->fxp;
		}
	}
	if ((e->f2t == FX_SPEED && e->f2p < 0x20) || e->f2t == FX_S3M_SPEED) {
		if (e->f2p) {
			p->speed = e->f2p;
		}
	}

	/* Delay event read */
	if (e->fxt == FX_EXTENDED && MSN(e->fxp) == EX_DELAY && LSN(e->fxp)) {
		xc->delay = LSN(e->fxp) + 1;
		goto do_delay;
	}
	if (e->f2t == FX_EXTENDED && MSN(e->f2p) == EX_DELAY && LSN(e->f2p)) {
		xc->delay = LSN(e->f2p) + 1;
		goto do_delay;
	}

	return 0;

    do_delay:
	memcpy(&xc->delayed_event, e, sizeof (struct xmp_event));

	if (e->ins) {
		xc->delayed_ins = e->ins;
	}

	if (HAS_QUIRK(QUIRK_RTDELAY)) {
		if (e->vol == 0 && e->f2t == 0 && e->ins == 0 && e->note != XMP_KEY_OFF)
			xc->delayed_event.vol = xc->volume + 1;
		if (e->note == 0)
			xc->delayed_event.note = xc->key + 1;
		if (e->ins == 0)
			xc->delayed_event.ins = xc->old_ins;
	}

	return 1;
}

static inline void read_row(struct context_data *ctx, int pat, int row)
{
	int chn;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct player_data *p = &ctx->p;
	struct flow_control *f = &p->flow;
	struct xmp_event ev;

	for (chn = 0; chn < mod->chn; chn++) {
		const int num_rows = mod->xxt[TRACK_NUM(pat, chn)]->rows;
		if (row < num_rows) {
			memcpy(&ev, &EVENT(pat, chn, row), sizeof(ev));
		} else {
			memset(&ev, 0, sizeof(ev));
		}

		if (ev.note == XMP_KEY_OFF) {
			int env_on = 0;
			int ins = ev.ins - 1;

			if (IS_VALID_INSTRUMENT(ins) &&
			    (mod->xxi[ins].aei.flg & XMP_ENVELOPE_ON)) {
				env_on = 1;
			}

			if (ev.fxt == FX_EXTENDED && MSN(ev.fxp) == EX_DELAY) {
				if (ev.ins && (LSN(ev.fxp) || env_on)) {
					if (LSN(ev.fxp)) {
						ev.note = 0;
					}
					ev.fxp = ev.fxt = 0;
				}
			}
		}

		if (check_delay(ctx, &ev, chn) == 0) {
			if (!f->rowdelay_set || f->rowdelay > 0) {
				libxmp_read_event(ctx, &ev, chn);
#ifndef LIBXMP_CORE_PLAYER
				libxmp_med_hold_hack(ctx, pat, chn, row);
#endif
			}
		} else {
			if (IS_PLAYER_MODE_IT()) {
				/* Reset flags. See SlideDelay.it */
				p->xc_data[chn].flags = 0;
			}
		}
	}
}

static inline int get_channel_vol(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	int root;

	/* channel is a root channel */
	if (chn < p->virt.num_tracks)
		return p->channel_vol[chn];

	/* channel is invalid */
	if (chn >= p->virt.virt_channels)
		return 0;

	/* root is invalid */
	root = libxmp_virt_getroot(ctx, chn);
	if (root < 0)
		return 0;

	return p->channel_vol[root];
}

static int tremor_ft2(struct context_data *ctx, int chn, int finalvol)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];

	if (xc->tremor.count & 0x80) {
		if (TEST(TREMOR) && p->frame != 0) {
			xc->tremor.count &= ~0x20;
			if (xc->tremor.count == 0x80) {
				/* end of down cycle, set up counter for up  */
				xc->tremor.count = xc->tremor.up | 0xc0;
			} else if (xc->tremor.count == 0xc0) {
				/* end of up cycle, set up counter for down */
				xc->tremor.count = xc->tremor.down | 0x80;
			} else {
				xc->tremor.count--;
			}
		}

		if ((xc->tremor.count & 0xe0) == 0x80) {
			finalvol = 0;
		}
	}

	return finalvol;
}

static int tremor_s3m(struct context_data *ctx, int chn, int finalvol)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];

	if (TEST(TREMOR)) {
		if (xc->tremor.count == 0) {
			/* end of down cycle, set up counter for up  */
			xc->tremor.count = xc->tremor.up | 0x80;
		} else if (xc->tremor.count == 0x80) {
			/* end of up cycle, set up counter for down */
			xc->tremor.count = xc->tremor.down;
		}

		xc->tremor.count--;

		if (~xc->tremor.count & 0x80) {
			finalvol = 0;
		}
	}

	return finalvol;
}

/*
 * Update channel data
 */

#define DOENV_RELEASE ((TEST_NOTE(NOTE_RELEASE) || act == VIRT_ACTION_OFF))

static void process_volume(struct context_data *ctx, int chn, int act)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct channel_data *xc = &p->xc_data[chn];
	struct xmp_instrument *instrument;
	int finalvol;
	uint16 vol_envelope;
	int fade = 0;

	instrument = libxmp_get_instrument(ctx, xc->ins);

	/* Keyoff and fadeout */

	/* Keyoff event in IT doesn't reset fadeout (see jeff93.it)
	 * In XM it depends on envelope (see graff-strange_land.xm vs
	 * Decibelter - Cosmic 'Wegian Mamas.xm)
	 */
	if (HAS_QUIRK(QUIRK_KEYOFF)) {
		/* If IT, only apply fadeout on note release if we don't
		 * have envelope, or if we have envelope loop
		 */
		if (TEST_NOTE(NOTE_RELEASE) || act == VIRT_ACTION_OFF) {
			if ((~instrument->aei.flg & XMP_ENVELOPE_ON) ||
			    (instrument->aei.flg & XMP_ENVELOPE_LOOP)) {
				fade = 1;
			}
		}
	} else {
		if (~instrument->aei.flg & XMP_ENVELOPE_ON) {
			if (TEST_NOTE(NOTE_RELEASE)) {
				xc->fadeout = 0;
			}
		}

		if (TEST_NOTE(NOTE_RELEASE) || act == VIRT_ACTION_OFF) {
			fade = 1;
		}
	}

	if (TEST_NOTE(NOTE_FADEOUT) || act == VIRT_ACTION_FADE) {
		fade = 1;
	}

	if (fade) {
		if (xc->fadeout > xc->ins_fade) {
			xc->fadeout -= xc->ins_fade;
		} else {
			xc->fadeout = 0;
			SET_NOTE(NOTE_END);
		}
	}

	switch (check_envelope_fade(&instrument->aei, xc->v_idx)) {
	case -1:
		SET_NOTE(NOTE_END);
		/* Don't reset channel, we may have a tone portamento later
		 * virt_resetchannel(ctx, chn);
		 */
		break;
	case 0:
		break;
	default:
		if (HAS_QUIRK(QUIRK_ENVFADE)) {
			SET_NOTE(NOTE_FADEOUT);
		}
	}

	if (!TEST_PER(VENV_PAUSE)) {
		xc->v_idx = update_envelope(&instrument->aei, xc->v_idx,
			DOENV_RELEASE, TEST(KEY_OFF), IS_PLAYER_MODE_IT());
	}

	vol_envelope = get_envelope(&instrument->aei, xc->v_idx, 64);
	if (check_envelope_end(&instrument->aei, xc->v_idx)) {
		if (vol_envelope == 0) {
			SET_NOTE(NOTE_END);
		}
		SET_NOTE(NOTE_ENV_END);
	}

	/* If note ended in background channel, we can safely reset it */
	if (TEST_NOTE(NOTE_END) && chn >= p->virt.num_tracks) {
		libxmp_virt_resetchannel(ctx, chn);
		return;
	}

#ifndef LIBXMP_CORE_PLAYER
	finalvol = libxmp_extras_get_volume(ctx, xc);
#else
	finalvol = xc->volume;
#endif

	if (IS_PLAYER_MODE_IT()) {
		finalvol = xc->volume * (100 - xc->rvv) / 100;
	}

	if (TEST(TREMOLO)) {
		/* OpenMPT VibratoReset.mod */
		if (!is_first_frame(ctx) || !HAS_QUIRK(QUIRK_PROTRACK)) {
			finalvol += libxmp_lfo_get(ctx, &xc->tremolo.lfo, 0) / (1 << 6);
		}

		if (!is_first_frame(ctx) || HAS_QUIRK(QUIRK_VIBALL)) {
			libxmp_lfo_update(&xc->tremolo.lfo);
		}
	}

	CLAMP(finalvol, 0, m->volbase);

	finalvol = (finalvol * xc->fadeout) >> 6;	/* 16 bit output */

	finalvol = (uint32)(vol_envelope * p->gvol * xc->mastervol /
		m->gvolbase * ((int)finalvol * 0x40 / m->volbase)) >> 18;

	/* Apply channel volume */
	finalvol = finalvol * get_channel_vol(ctx, chn) / 100;

#ifndef LIBXMP_CORE_PLAYER
	/* Volume translation table (for PTM, ARCH, COCO) */
	if (m->vol_table) {
		finalvol = m->volbase == 0xff ?
		    m->vol_table[finalvol >> 2] << 2 :
		    m->vol_table[finalvol >> 4] << 4;
	}
#endif

	if (HAS_QUIRK(QUIRK_INSVOL)) {
		finalvol = (finalvol * instrument->vol * xc->gvl) >> 12;
	}

	if (IS_PLAYER_MODE_FT2()) {
		finalvol = tremor_ft2(ctx, chn, finalvol);
	} else {
		finalvol = tremor_s3m(ctx, chn, finalvol);
	}

	if (chn < m->mod.chn) {
		finalvol = finalvol * p->master_vol / 100;
	} else {
		finalvol = finalvol * p->smix_vol / 100;
	}

	xc->info_finalvol = TEST_NOTE(NOTE_SAMPLE_END) ? 0 : finalvol;

	libxmp_virt_setvol(ctx, chn, finalvol);

	/* Check Amiga split channel */
	if (xc->split) {
		libxmp_virt_setvol(ctx, xc->pair, finalvol);
	}
}

static void process_frequency(struct context_data *ctx, int chn, int act)
{
#ifndef LIBXMP_CORE_DISABLE_IT
	struct mixer_data *s = &ctx->s;
#endif
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct channel_data *xc = &p->xc_data[chn];
	struct xmp_instrument *instrument;
	double period, vibrato;
	int linear_bend;
	int frq_envelope;
	int arp;
#ifndef LIBXMP_CORE_DISABLE_IT
	int cutoff, resonance;
#endif

	instrument = libxmp_get_instrument(ctx, xc->ins);

	if (!TEST_PER(FENV_PAUSE)) {
		xc->f_idx = update_envelope(&instrument->fei, xc->f_idx,
			DOENV_RELEASE, TEST(KEY_OFF), IS_PLAYER_MODE_IT());
	}
	frq_envelope = get_envelope(&instrument->fei, xc->f_idx, 0);

#ifndef LIBXMP_CORE_PLAYER
	/* Do note slide */

	if (TEST(NOTE_SLIDE)) {
		if (xc->noteslide.count == 0) {
			xc->note += xc->noteslide.slide;
			xc->period = libxmp_note_to_period(ctx, xc->note,
					xc->finetune, xc->per_adj);
			xc->noteslide.count = xc->noteslide.speed;
		}
		xc->noteslide.count--;

		libxmp_virt_setnote(ctx, chn, xc->note);
	}
#endif

	/* Instrument vibrato */
	vibrato = 1.0 * libxmp_lfo_get(ctx, &xc->insvib.lfo, 1) /
				(4096 * (1 + xc->insvib.sweep));
	libxmp_lfo_update(&xc->insvib.lfo);
	if (xc->insvib.sweep > 1) {
		xc->insvib.sweep -= 2;
	} else {
		xc->insvib.sweep = 0;
	}

	/* Vibrato */
	if (TEST(VIBRATO) || TEST_PER(VIBRATO)) {
		/* OpenMPT VibratoReset.mod */
		if (!is_first_frame(ctx) || !HAS_QUIRK(QUIRK_PROTRACK)) {
			int shift = HAS_QUIRK(QUIRK_VIBHALF) ? 10 : 9;
			int vib = libxmp_lfo_get(ctx, &xc->vibrato.lfo, 1) / (1 << shift);

			if (HAS_QUIRK(QUIRK_VIBINV)) {
				vibrato -= vib;
			} else {
				vibrato += vib;
			}
		}

		if (!is_first_frame(ctx) || HAS_QUIRK(QUIRK_VIBALL)) {
			libxmp_lfo_update(&xc->vibrato.lfo);
		}
	}

	period = xc->period;
#ifndef LIBXMP_CORE_PLAYER
	period += libxmp_extras_get_period(ctx, xc);
#endif

	/* Sanity check */
	if (period < 0.1) {
		period = 0.1;
	} 

	/* Arpeggio */

	if (HAS_QUIRK(QUIRK_FT2BUGS)) {
		arp = ft2_arpeggio(ctx, xc);
	} else {
		arp = xc->arpeggio.val[xc->arpeggio.count];
	}

	/* Pitch bend */

 	/* From OpenMPT PeriodLimit.s3m:
	 * "ScreamTracker 3 limits the final output period to be at least 64,
	 *  i.e. when playing a note that is too high or when sliding the
	 *  period lower than 64, the output period will simply be clamped to
	 *  64. However, when reaching a period of 0 through slides, the
	 *  output on the channel should be stopped."
	 */
	/* ST3 uses periods*4, so the limit is 16. Adjusted to the exact
	 * A6 value because we compute periods in floating point.
	 */
	if (HAS_QUIRK(QUIRK_ST3BUGS)) {
		if (period < 16.239270) {	/* A6 */
			period = 16.239270;
		}
	}

	linear_bend = libxmp_period_to_bend(ctx, period + vibrato, xc->note,
							xc->per_adj);

	if (TEST_NOTE(NOTE_GLISSANDO) && TEST(TONEPORTA)) {
		if (linear_bend > 0) {
			linear_bend = (linear_bend + 6400) / 12800 * 12800;
		} else if (linear_bend < 0) {
			linear_bend = (linear_bend - 6400) / 12800 * 12800;
		}
	}

	if (HAS_QUIRK(QUIRK_FT2BUGS)) {
		if  (arp) {
			/* OpenMPT ArpSlide.xm */
			linear_bend = linear_bend / 12800 * 12800 +
							xc->finetune * 100;

			/* OpenMPT ArpeggioClamp.xm */
			if (xc->note + arp > 107) {
				if (p->speed - (p->frame % p->speed) > 0) {
					arp = 108 - xc->note;
				}
			}
		}
	}
	
	/* Envelope */

	if (xc->f_idx >= 0 && (~instrument->fei.flg & XMP_ENVELOPE_FLT)) {
		/* IT pitch envelopes are always linear, even in Amiga period
		 * mode. Each unit in the envelope scale is 1/25 semitone.
		 */
		linear_bend += frq_envelope << 7;
	}

	/* Arpeggio */

	if (arp != 0) {
		linear_bend += (100 << 7) * arp;

		/* OpenMPT ArpWrapAround.mod */
		if (HAS_QUIRK(QUIRK_PROTRACK)) {
			if (xc->note + arp > MAX_NOTE_MOD + 1) {
				linear_bend -= 12800 * (3 * 12);
			} else if (xc->note + arp > MAX_NOTE_MOD) {
				libxmp_virt_setvol(ctx, chn, 0);
			}
		}
	}


#ifndef LIBXMP_CORE_PLAYER
	linear_bend += libxmp_extras_get_linear_bend(ctx, xc);
#endif

	period = libxmp_note_to_period_mix(xc->note, linear_bend);
	libxmp_virt_setperiod(ctx, chn, period);

	/* For xmp_get_frame_info() */
	xc->info_pitchbend = linear_bend >> 7;
	xc->info_period = (int) (period * 4096);

	if (IS_PERIOD_MODRNG()) {
		CLAMP(xc->info_period,
            (int) (libxmp_note_to_period(ctx, MAX_NOTE_MOD, xc->finetune, 0) * 4096),
            (int) (libxmp_note_to_period(ctx, MIN_NOTE_MOD, xc->finetune, 0) * 4096));
	} else if (xc->info_period < (1 << 12)) {
		xc->info_period = (1 << 12);
	}


#ifndef LIBXMP_CORE_DISABLE_IT

	/* Process filter */

	if (!HAS_QUIRK(QUIRK_FILTER)) {
		return;
	}

	if (xc->f_idx >= 0 && (instrument->fei.flg & XMP_ENVELOPE_FLT)) {
		if (frq_envelope < 0xfe) {
			xc->filter.envelope = frq_envelope;
		}
		cutoff = xc->filter.cutoff * xc->filter.envelope >> 8;
	} else {
		cutoff = xc->filter.cutoff;
	}
	resonance = xc->filter.resonance;

	if (cutoff > 0xff) {
		cutoff = 0xff;
	} else if (cutoff < 0xff) {
		int a0, b0, b1;
		libxmp_filter_setup(s->freq, cutoff, resonance, &a0, &b0, &b1);
		libxmp_virt_seteffect(ctx, chn, DSP_EFFECT_FILTER_A0, a0);
		libxmp_virt_seteffect(ctx, chn, DSP_EFFECT_FILTER_B0, b0);
		libxmp_virt_seteffect(ctx, chn, DSP_EFFECT_FILTER_B1, b1);
		libxmp_virt_seteffect(ctx, chn, DSP_EFFECT_RESONANCE, resonance);
	}

	/* Always set cutoff */
	libxmp_virt_seteffect(ctx, chn, DSP_EFFECT_CUTOFF, cutoff);

#endif
}

static void process_pan(struct context_data *ctx, int chn, int act)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct mixer_data *s = &ctx->s;
	struct channel_data *xc = &p->xc_data[chn];
	struct xmp_instrument *instrument;
	int finalpan, panbrello = 0;
	int pan_envelope;
	int channel_pan;

	instrument = libxmp_get_instrument(ctx, xc->ins);

	if (!TEST_PER(PENV_PAUSE)) {
		xc->p_idx = update_envelope(&instrument->pei, xc->p_idx,
			DOENV_RELEASE, TEST(KEY_OFF), IS_PLAYER_MODE_IT());
	}
	pan_envelope = get_envelope(&instrument->pei, xc->p_idx, 32);

#ifndef LIBXMP_CORE_DISABLE_IT
	if (TEST(PANBRELLO)) {
		panbrello = libxmp_lfo_get(ctx, &xc->panbrello.lfo, 0) / 512;
		if (is_first_frame(ctx)) {
			libxmp_lfo_update(&xc->panbrello.lfo);
		}
	}
#endif

	channel_pan = xc->pan.val;

#if 0
#ifdef LIBXMP_PAULA_SIMULATOR
	/* Always use 100% pan separation in Amiga mode */
	if (p->flags & XMP_FLAGS_A500) {
		if (IS_AMIGA_MOD()) {
			channel_pan = channel_pan < 0x80 ? 0 : 0xff;
		}
	}
#endif
#endif

	finalpan = channel_pan + panbrello + (pan_envelope - 32) *
				(128 - abs(xc->pan.val - 128)) / 32;

	if (IS_PLAYER_MODE_IT()) {
		finalpan = finalpan + xc->rpv * 4;
	}

	CLAMP(finalpan, 0, 255);

	if (s->format & XMP_FORMAT_MONO || xc->pan.surround) {
		finalpan = 0;
	} else {
		finalpan = (finalpan - 0x80) * s->mix / 100;
	}

	xc->info_finalpan = finalpan + 0x80;

	if (xc->pan.surround) {
		libxmp_virt_setpan(ctx, chn, PAN_SURROUND);
	} else {
		libxmp_virt_setpan(ctx, chn, finalpan);
	}
}

static void update_volume(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
#ifndef LIBXMP_CORE_DISABLE_IT
	struct flow_control *f = &p->flow;
#endif
	struct channel_data *xc = &p->xc_data[chn];

	/* Volume slides happen in all frames but the first, except when the
	 * "volume slide on all frames" flag is set.
	 */
	if (p->frame % p->speed != 0 || HAS_QUIRK(QUIRK_VSALL)) {
		if (TEST(GVOL_SLIDE)) {
			p->gvol += xc->gvol.slide;
		}

		if (TEST(VOL_SLIDE) || TEST_PER(VOL_SLIDE)) {
			xc->volume += xc->vol.slide;
		}

#ifndef LIBXMP_CORE_PLAYER
		if (TEST_PER(VOL_SLIDE)) {
			if (xc->vol.slide > 0 && xc->volume > m->volbase) {
				xc->volume = m->volbase;
				RESET_PER(VOL_SLIDE);
			}
			if (xc->vol.slide < 0 && xc->volume < 0) {
				xc->volume = 0;
				RESET_PER(VOL_SLIDE);
			}
		}
#endif

		if (TEST(VOL_SLIDE_2)) {
			xc->volume += xc->vol.slide2;
		}
		if (TEST(TRK_VSLIDE)) {
			xc->mastervol += xc->trackvol.slide;
		}
	}

	if (p->frame % p->speed == 0) {
		/* Process "fine" effects */
		if (TEST(FINE_VOLS)) {
			xc->volume += xc->vol.fslide;
		}

#ifndef LIBXMP_CORE_DISABLE_IT
		if (TEST(FINE_VOLS_2)) {
			/* OpenMPT FineVolColSlide.it:
			 * Unlike fine volume slides in the effect column,
			 * fine volume slides in the volume column are only
			 * ever executed on the first tick -- not on multiples
			 * of the first tick if there is a pattern delay. 
			 */
			if (!f->rowdelay_set || f->rowdelay_set & 2) {
				xc->volume += xc->vol.fslide2;
			}
		}
		f->rowdelay_set &= ~2;
#endif

		if (TEST(TRK_FVSLIDE)) {
			xc->mastervol += xc->trackvol.fslide;
		}

		if (TEST(GVOL_SLIDE)) {
			p->gvol += xc->gvol.fslide;
		}
	}

	/* Clamp volumes */
	CLAMP(xc->volume, 0, m->volbase);
	CLAMP(p->gvol, 0, m->gvolbase);
	CLAMP(xc->mastervol, 0, m->volbase);

	if (xc->split) {
		p->xc_data[xc->pair].volume = xc->volume;
	}
}

static void update_frequency(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct channel_data *xc = &p->xc_data[chn];

	if (!is_first_frame(ctx) || HAS_QUIRK(QUIRK_PBALL)) {
		if (TEST(PITCHBEND) || TEST_PER(PITCHBEND)) {
			xc->period += xc->freq.slide;
			if (HAS_QUIRK(QUIRK_PROTRACK)) {
				xc->porta.target = xc->period;
			}
		}

		/* Do tone portamento */
		if (TEST(TONEPORTA) || TEST_PER(TONEPORTA)) {
			if (xc->porta.target > 0) {
				int end = 0;
				if (xc->porta.dir > 0) {
					xc->period += xc->porta.slide;
					if (xc->period >= xc->porta.target)
						end = 1;
				} else {
					xc->period -= xc->porta.slide;
					if (xc->period <= xc->porta.target)
						end = 1;
				}

				if (end) {
					/* reached end */
					xc->period = xc->porta.target;
					xc->porta.dir = 0;
					RESET(TONEPORTA);
					RESET_PER(TONEPORTA);

					if (HAS_QUIRK(QUIRK_PROTRACK)) {
						xc->porta.target = -1;
					}
				}
			}
		} 
	}

	if (is_first_frame(ctx)) {
		if (TEST(FINE_BEND)) {
			xc->period += xc->freq.fslide;
		}

#ifndef LIBXMP_CORE_PLAYER
		if (TEST(FINE_NSLIDE)) {
			xc->note += xc->noteslide.fslide;
			xc->period = libxmp_note_to_period(ctx, xc->note,
				xc->finetune, xc->per_adj);
		}
#endif
	}

	switch (m->period_type) {
	case PERIOD_LINEAR:
		CLAMP(xc->period, MIN_PERIOD_L, MAX_PERIOD_L);
		break;
	case PERIOD_MODRNG:
		CLAMP(xc->period,
			libxmp_note_to_period(ctx, MAX_NOTE_MOD, xc->finetune, 0),
			libxmp_note_to_period(ctx, MIN_NOTE_MOD, xc->finetune, 0));
		break;
	}

	xc->arpeggio.count++;
	xc->arpeggio.count %= xc->arpeggio.size;

	/* Check for invalid periods (from Toru Egashira's NSPmod)
	 * panic.s3m has negative periods
	 * ambio.it uses low (~8) period values
	 */
	if (xc->period < 0.25) {
		libxmp_virt_setvol(ctx, chn, 0);
	}
}

static void update_pan(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];

	if (TEST(PAN_SLIDE)) {
		if (is_first_frame(ctx)) {
			xc->pan.val += xc->pan.fslide;
		} else {
			xc->pan.val += xc->pan.slide;
		}

		if (xc->pan.val < 0) {
			xc->pan.val = 0;
		} else if (xc->pan.val > 0xff) {
			xc->pan.val = 0xff;
		}
	}
}

static void play_channel(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct smix_data *smix = &ctx->smix;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct channel_data *xc = &p->xc_data[chn];
	int act;

	xc->info_finalvol = 0;

#ifndef LIBXMP_CORE_DISABLE_IT
	/* IT tempo slide */
	if (!is_first_frame(ctx) && TEST(TEMPO_SLIDE)) {
		p->bpm += xc->tempo.slide;
		CLAMP(p->bpm, 0x20, 0xff);
	}
#endif

	/* Do delay */
	if (xc->delay > 0) {
		if (--xc->delay == 0) {
			libxmp_read_event(ctx, &xc->delayed_event, chn);
		}
	}

	act = libxmp_virt_cstat(ctx, chn);
	if (act == VIRT_INVALID) {
		/* We need this to keep processing global volume slides */
		update_volume(ctx, chn);
		return;
	}

	if (p->frame == 0 && act != VIRT_ACTIVE) {
		if (!IS_VALID_INSTRUMENT_OR_SFX(xc->ins) || act == VIRT_ACTION_CUT) {
			libxmp_virt_resetchannel(ctx, chn);
			return;
		}
	}

	if (!IS_VALID_INSTRUMENT_OR_SFX(xc->ins))
		return;

#ifndef LIBXMP_CORE_PLAYER
	libxmp_play_extras(ctx, xc, chn);
#endif

	/* Do cut/retrig */
	if (TEST(RETRIG)) {
		int cond = HAS_QUIRK(QUIRK_S3MRTG) ?
				--xc->retrig.count <= 0 :
				--xc->retrig.count == 0;

		if (cond) {
			if (xc->retrig.type < 0x10) {
				/* don't retrig on cut */
				libxmp_virt_voicepos(ctx, chn, 0);
			} else {
				SET_NOTE(NOTE_END);
			}
			xc->volume += rval[xc->retrig.type].s;
			xc->volume *= rval[xc->retrig.type].m;
			xc->volume /= rval[xc->retrig.type].d;
                	xc->retrig.count = LSN(xc->retrig.val);
		}
        }

	/* Do keyoff */
	if (xc->keyoff) {
		if (--xc->keyoff == 0)
			SET_NOTE(NOTE_RELEASE);
	}

	libxmp_virt_release(ctx, chn, TEST_NOTE(NOTE_RELEASE));

	process_volume(ctx, chn, act);
	process_frequency(ctx, chn, act);
	process_pan(ctx, chn, act);

	update_volume(ctx, chn);
	update_frequency(ctx, chn);
	update_pan(ctx, chn);

#ifndef LIBXMP_CORE_PLAYER
	if (HAS_QUIRK(QUIRK_PROTRACK) && xc->ins < mod->ins) {
		update_invloop(m, xc);
	}
#endif

	if (TEST_NOTE(NOTE_SUSEXIT)) {
		SET_NOTE(NOTE_RELEASE);
	}

	xc->info_position = (int) libxmp_virt_getvoicepos(ctx, chn);
}

/*
 * Event injection
 */

static void inject_event(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct smix_data *smix = &ctx->smix;
	int chn;
	
	for (chn = 0; chn < mod->chn + smix->chn; chn++) {
		struct xmp_event *e = &p->inject_event[chn];
		if (e->_flag > 0) {
			libxmp_read_event(ctx, e, chn);
			e->_flag = 0;
		}
	}
}

/*
 * Sequencing
 */

static void next_order(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct flow_control *f = &p->flow;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int mark;

	do {
    		p->ord++;

		/* Restart module */
		mark = HAS_QUIRK(QUIRK_MARKER) && mod->xxo[p->ord] == 0xff;
		if (p->ord >= mod->len || mark) {
			if (mod->rst > mod->len ||
			    mod->xxo[mod->rst] >= mod->pat ||
			    p->ord < m->seq_data[p->sequence].entry_point) {
				p->ord = m->seq_data[p->sequence].entry_point;
			} else {
				if (libxmp_get_sequence(ctx, mod->rst) == p->sequence) {
					p->ord = mod->rst;
				} else {
					p->ord = m->seq_data[p->sequence].entry_point;
				}
			}

			p->gvol = m->xxo_info[p->ord].gvl;
		}
	} while (mod->xxo[p->ord] >= mod->pat);

	p->current_time = m->xxo_info[p->ord].time;

	f->num_rows = mod->xxp[mod->xxo[p->ord]]->rows;
	if (f->jumpline >= f->num_rows)
		f->jumpline = 0;
	p->row = f->jumpline;
	f->jumpline = 0;

	p->pos = p->ord;
	p->frame = 0;

#ifndef LIBXMP_CORE_PLAYER
	/* Reset persistent effects at new pattern */
	if (HAS_QUIRK(QUIRK_PERPAT)) {
		int chn;
		for (chn = 0; chn < mod->chn; chn++) {
			p->xc_data[chn].per_flags = 0;
		}
	}
#endif
}

static void next_row(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct flow_control *f = &p->flow;

	p->frame = 0;
	f->delay = 0;

	if (f->pbreak) {
		f->pbreak = 0;

		if (f->jump != -1) {
			p->ord = f->jump - 1;
			f->jump = -1;
		}

		next_order(ctx);
	} else {
		if (f->loop_chn) {
			p->row = f->loop[f->loop_chn - 1].start - 1;
			f->loop_chn = 0;
		}
	
		if (f->rowdelay == 0) {
			p->row++;
			f->rowdelay_set = 0;
		} else {
			f->rowdelay--;
		}
	
		/* check end of pattern */
		if (p->row >= f->num_rows) {
			next_order(ctx);
		}
	}
}

#ifndef LIBXMP_CORE_DISABLE_IT

/*
 * Set note action for libxmp_virt_pastnote
 */
void libxmp_player_set_release(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];

	SET_NOTE(NOTE_RELEASE);
}

void libxmp_player_set_fadeout(struct context_data *ctx, int chn)
{
	struct player_data *p = &ctx->p;
	struct channel_data *xc = &p->xc_data[chn];

	SET_NOTE(NOTE_FADEOUT);
}

#endif

static void update_from_ord_info(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct ord_data *oinfo = &m->xxo_info[p->ord];

	if (oinfo->speed)
		p->speed = oinfo->speed;
	p->bpm = oinfo->bpm;
	p->gvol = oinfo->gvl;
	p->current_time = oinfo->time;
	p->frame_time = m->time_factor * m->rrate / p->bpm;

#ifndef LIBXMP_CORE_PLAYER
	p->st26_speed = oinfo->st26_speed;
#endif
}

int xmp_start_player(xmp_context opaque, int rate, int format)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct player_data *p = &ctx->p;
	struct smix_data *smix = &ctx->smix;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct flow_control *f = &p->flow;
	int i;
	int ret = 0;

	if (rate < XMP_MIN_SRATE || rate > XMP_MAX_SRATE)
		return -XMP_ERROR_INVALID;

	if (ctx->state < XMP_STATE_LOADED)
		return -XMP_ERROR_STATE;

	if (ctx->state > XMP_STATE_LOADED)
		xmp_end_player(opaque);

	if (libxmp_mixer_on(ctx, rate, format, m->c4rate) < 0)
		return -XMP_ERROR_INTERNAL;

	p->master_vol = 100;
	p->smix_vol = 100;
	p->gvol = m->volbase;
	p->pos = p->ord = 0;
	p->frame = -1;
	p->row = 0;
	p->current_time = 0;
	p->loop_count = 0;
	p->sequence = 0;

	/* Unmute all channels and set default volume */
	for (i = 0; i < XMP_MAX_CHANNELS; i++) {
		p->channel_mute[i] = 0;
		p->channel_vol[i] = 100;
	}

	/* Skip invalid patterns at start (the seventh laboratory.it) */
	while (p->ord < mod->len && mod->xxo[p->ord] >= mod->pat) {
		p->ord++;
	}
	/* Check if all positions skipped */
	if (p->ord >= mod->len) {
		mod->len = 0;
	}

	if (mod->len == 0 || mod->chn == 0) {
		/* set variables to sane state */
		p->ord = p->scan[0].ord = 0;
		p->row = p->scan[0].row = 0;
		f->end_point = 0;
		f->num_rows = 0;
	} else {
		f->num_rows = mod->xxp[mod->xxo[p->ord]]->rows;
		f->end_point = p->scan[0].num;
	}

	update_from_ord_info(ctx);

	if (libxmp_virt_on(ctx, mod->chn + smix->chn) != 0) {
		ret = -XMP_ERROR_INTERNAL;
		goto err;
	}

	f->delay = 0;
	f->jumpline = 0;
	f->jump = -1;
	f->pbreak = 0;
	f->rowdelay_set = 0;

	f->loop = (struct pattern_loop *)calloc(p->virt.virt_channels, sizeof(struct pattern_loop));
	if (f->loop == NULL) {
		ret = -XMP_ERROR_SYSTEM;
		goto err;
	}

	p->xc_data = (struct channel_data *)calloc(p->virt.virt_channels, sizeof(struct channel_data));
	if (p->xc_data == NULL) {
		ret = -XMP_ERROR_SYSTEM;
		goto err1;
	}

#ifndef LIBXMP_CORE_PLAYER
	for (i = 0; i < p->virt.virt_channels; i++) {
		struct channel_data *xc = &p->xc_data[i];
		if (libxmp_new_channel_extras(ctx, xc) < 0)
			goto err2;
	}
#endif
	reset_channels(ctx);

	ctx->state = XMP_STATE_PLAYING;

	return 0;

#ifndef LIBXMP_CORE_PLAYER
    err2:
	free(p->xc_data);
#endif
    err1:
	free(f->loop);
    err:
	return ret;
}

static void check_end_of_module(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct flow_control *f = &p->flow;

	/* check end of module */
	if (p->ord == p->scan[p->sequence].ord &&
			p->row == p->scan[p->sequence].row) {
		if (f->end_point == 0) {
			p->loop_count++;
			f->end_point = p->scan[p->sequence].num;
			/* return -1; */
		}
		f->end_point--;
	}
}

int xmp_play_frame(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct flow_control *f = &p->flow;
	int i;

	if (ctx->state < XMP_STATE_PLAYING)
		return -XMP_ERROR_STATE;

	if (mod->len <= 0) {
		return -XMP_END;
	}

	if (HAS_QUIRK(QUIRK_MARKER) && mod->xxo[p->ord] == 0xff) {
		return -XMP_END;
	}

	/* check reposition */
	if (p->ord != p->pos) {
		int start = m->seq_data[p->sequence].entry_point;

		if (p->pos == -2) {		/* set by xmp_module_stop */
			return -XMP_END;	/* that's all folks */
		}

		if (p->pos == -1) {
			/* restart sequence */
			p->pos = start;
		}

		if (p->pos == start) {
			f->end_point = p->scan[p->sequence].num;
		}

		/* Check if lands after a loop point */
		if (p->pos > p->scan[p->sequence].ord) {
			f->end_point = 0;
		}

		f->jumpline = 0;
		f->jump = -1;

		p->ord = p->pos - 1;

		/* Stay inside our subsong */
		if (p->ord < start) {
			p->ord = start - 1;
		}

		next_order(ctx);

		update_from_ord_info(ctx);

		libxmp_virt_reset(ctx);
		reset_channels(ctx);
	} else {
		p->frame++;
		if (p->frame >= (p->speed * (1 + f->delay))) {
			/* If break during pattern delay, next row is skipped.
			 * See corruption.mod order 1D (pattern 0D) last line:
			 * EE2 + D31 ignores D00 in order 1C line 31. Reported
			 * by The Welder <welder@majesty.net>, Jan 14 2012
			 */
			if (HAS_QUIRK(QUIRK_PROTRACK) && f->delay && f->pbreak)
			{
				next_row(ctx);
				check_end_of_module(ctx);
			}
			next_row(ctx);
		}
	}

	for (i = 0; i < mod->chn; i++) {
		struct channel_data *xc = &p->xc_data[i];
		RESET(KEY_OFF);
	}

	/* check new row */

	if (p->frame == 0) {			/* first frame in row */
		check_end_of_module(ctx);
		read_row(ctx, mod->xxo[p->ord], p->row);

#ifndef LIBXMP_CORE_PLAYER
		if (p->st26_speed) {
			if  (p->st26_speed & 0x10000) {
				p->speed = (p->st26_speed & 0xff00) >> 8;
			} else {
				p->speed = p->st26_speed & 0xff;
			}
			p->st26_speed ^= 0x10000;
		}
#endif
	}

	inject_event(ctx);

	/* play_frame */
	for (i = 0; i < p->virt.virt_channels; i++) {
		play_channel(ctx, i);
	}

	p->frame_time = m->time_factor * m->rrate / p->bpm;
	p->current_time += p->frame_time;

	libxmp_mixer_softmixer(ctx);

	return 0;
}

int xmp_play_buffer(xmp_context opaque, void *out_buffer, int size, int loop)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct player_data *p = &ctx->p;
	int ret = 0, filled = 0, copy_size;
	struct xmp_frame_info fi;

	/* Reset internal state
	 * Syncs buffer start with frame start */
	if (out_buffer == NULL) {
		p->loop_count = 0;
		p->buffer_data.consumed = 0;
		p->buffer_data.in_size = 0;
		return 0;
	}

	if (ctx->state < XMP_STATE_PLAYING)
		return -XMP_ERROR_STATE;

	/* Fill buffer */
	while (filled < size) {
		/* Check if buffer full */
		if (p->buffer_data.consumed == p->buffer_data.in_size) {
			ret = xmp_play_frame(opaque);
			xmp_get_frame_info(opaque, &fi);

			/* Check end of module */
			if (ret < 0 || (loop > 0 && fi.loop_count >= loop)) {
				/* Start of frame, return end of replay */
				if (filled == 0) {
					p->buffer_data.consumed = 0;
					p->buffer_data.in_size = 0;
					return -1;
				}

				/* Fill remaining of this buffer */
				memset((char *)out_buffer + filled, 0, size - filled);
				return 0;
			}

			p->buffer_data.consumed = 0;
			p->buffer_data.in_buffer = (char *)fi.buffer;
			p->buffer_data.in_size = fi.buffer_size;
		}

		/* Copy frame data to user buffer */
		copy_size = MIN(size - filled, p->buffer_data.in_size -
					p->buffer_data.consumed);
		memcpy((char *)out_buffer + filled, p->buffer_data.in_buffer +
					p->buffer_data.consumed, copy_size);
		p->buffer_data.consumed += copy_size;
		filled += copy_size;
	}

	return ret;
}
    
void xmp_end_player(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct player_data *p = &ctx->p;
	struct flow_control *f = &p->flow;
#ifndef LIBXMP_CORE_PLAYER
	struct channel_data *xc;
	int i;
#endif

	if (ctx->state < XMP_STATE_PLAYING)
		return;

	ctx->state = XMP_STATE_LOADED;

#ifndef LIBXMP_CORE_PLAYER
	/* Free channel extras */
	for (i = 0; i < p->virt.virt_channels; i++) {
		xc = &p->xc_data[i];
		libxmp_release_channel_extras(ctx, xc);
	}
#endif

	libxmp_virt_off(ctx);

	free(p->xc_data);
	free(f->loop);

	p->xc_data = NULL;
	f->loop = NULL;

	libxmp_mixer_off(ctx);
}

void xmp_get_module_info(xmp_context opaque, struct xmp_module_info *info)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;

	if (ctx->state < XMP_STATE_LOADED)
		return;

	memcpy(info->md5, m->md5, 16);
	info->mod = mod;
	info->comment = m->comment;
	info->num_sequences = m->num_sequences;
	info->seq_data = m->seq_data;
	info->vol_base = m->volbase;
}

void xmp_get_frame_info(xmp_context opaque, struct xmp_frame_info *info)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct player_data *p = &ctx->p;
	struct mixer_data *s = &ctx->s;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int chn, i;

	if (ctx->state < XMP_STATE_LOADED)
		return;

	chn = mod->chn;

	if (p->pos >= 0 && p->pos < mod->len) {
		info->pos = p->pos;
	} else {
		info->pos = 0;
	}

	info->pattern = mod->xxo[info->pos];

	if (info->pattern < mod->pat) {
		info->num_rows = mod->xxp[info->pattern]->rows;
	} else {
		info->num_rows = 0;
	}

	info->row = p->row;
	info->frame = p->frame;
	info->speed = p->speed;
	info->bpm = p->bpm;
	info->total_time = p->scan[p->sequence].time;
	info->frame_time = (int) (p->frame_time * 1000);
	info->time = (int) p->current_time;
	info->buffer = s->buffer;

	info->total_size = XMP_MAX_FRAMESIZE;
	info->buffer_size = s->ticksize;
	if (~s->format & XMP_FORMAT_MONO) {
		info->buffer_size *= 2;
	}
	if (~s->format & XMP_FORMAT_8BIT) {
		info->buffer_size *= 2;
	}

	info->volume = p->gvol;
	info->loop_count = p->loop_count;
	info->virt_channels = p->virt.virt_channels;
	info->virt_used = p->virt.virt_used;

	info->sequence = p->sequence;

	if (p->xc_data != NULL) {
		for (i = 0; i < chn; i++) {
			struct channel_data *c = &p->xc_data[i];
			struct xmp_channel_info *ci = &info->channel_info[i];
			struct xmp_track *track;
			struct xmp_event *event;
			int trk;
	
			ci->note = c->key;
			ci->pitchbend = c->info_pitchbend;
			ci->period = c->info_period;
			ci->position = c->info_position;
			ci->instrument = c->ins;
			ci->sample = c->smp;
			ci->volume = c->info_finalvol >> 4;
			ci->pan = c->info_finalpan;
			ci->reserved = 0;
			memset(&ci->event, 0, sizeof(*event));
	
			if (info->pattern < mod->pat && info->row < info->num_rows) {
				trk = mod->xxp[info->pattern]->index[i];
				track = mod->xxt[trk];
				if (info->row < track->rows) {
					event = &track->event[info->row];
					memcpy(&ci->event, event, sizeof(*event));
				}
			}
		}
	}
}
