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
#include <string.h>
#include <math.h>
#include "common.h"
#include "virtual.h"
#include "mixer.h"
#include "period.h"
#include "player.h"	/* for set_sample_end() */

#ifdef LIBXMP_PAULA_SIMULATOR
#include "paula.h"
#endif


#define FLAG_16_BITS	0x01
#define FLAG_STEREO	0x02
#define FLAG_FILTER	0x04
#define FLAG_ACTIVE	0x10
/* #define FLAG_SYNTH	0x20 */
#define FIDX_FLAGMASK	(FLAG_16_BITS | FLAG_STEREO | FLAG_FILTER)

#define DOWNMIX_SHIFT	 12
#define LIM8_HI		 127
#define LIM8_LO		-128
#define LIM16_HI	 32767
#define LIM16_LO	-32768

#define MIX_FN(x) void libxmp_mix_##x(struct mixer_voice *, int32 *, int, int, int, int, int, int, int)

MIX_FN(mono_8bit_nearest);
MIX_FN(mono_8bit_linear);
MIX_FN(mono_16bit_nearest);
MIX_FN(mono_16bit_linear);
MIX_FN(stereo_8bit_nearest);
MIX_FN(stereo_8bit_linear);
MIX_FN(stereo_16bit_nearest);
MIX_FN(stereo_16bit_linear);
MIX_FN(mono_8bit_spline);
MIX_FN(mono_16bit_spline);
MIX_FN(stereo_8bit_spline);
MIX_FN(stereo_16bit_spline);

#ifndef LIBXMP_CORE_DISABLE_IT
MIX_FN(mono_8bit_linear_filter);
MIX_FN(mono_16bit_linear_filter);
MIX_FN(stereo_8bit_linear_filter);
MIX_FN(stereo_16bit_linear_filter);
MIX_FN(mono_8bit_spline_filter);
MIX_FN(mono_16bit_spline_filter);
MIX_FN(stereo_8bit_spline_filter);
MIX_FN(stereo_16bit_spline_filter);
#endif

#ifdef LIBXMP_PAULA_SIMULATOR
MIX_FN(mono_a500);
MIX_FN(mono_a500_filter);
MIX_FN(stereo_a500);
MIX_FN(stereo_a500_filter);
#endif

/* Mixers array index:
 *
 * bit 0: 0=8 bit sample, 1=16 bit sample
 * bit 1: 0=mono output, 1=stereo output
 * bit 2: 0=unfiltered, 1=filtered
 */

typedef void (*mixer_set[])(struct mixer_voice *, int32 *, int, int, int, int, int, int, int);

static mixer_set nearest_mixers = {
	libxmp_mix_mono_8bit_nearest,
	libxmp_mix_mono_16bit_nearest,
	libxmp_mix_stereo_8bit_nearest,
	libxmp_mix_stereo_16bit_nearest,

#ifndef LIBXMP_CORE_DISABLE_IT
	libxmp_mix_mono_8bit_nearest,
	libxmp_mix_mono_16bit_nearest,
	libxmp_mix_stereo_8bit_nearest,
	libxmp_mix_stereo_16bit_nearest,
#endif
};

static mixer_set linear_mixers = {
	libxmp_mix_mono_8bit_linear,
	libxmp_mix_mono_16bit_linear,
	libxmp_mix_stereo_8bit_linear,
	libxmp_mix_stereo_16bit_linear,

#ifndef LIBXMP_CORE_DISABLE_IT
	libxmp_mix_mono_8bit_linear_filter,
	libxmp_mix_mono_16bit_linear_filter,
	libxmp_mix_stereo_8bit_linear_filter,
	libxmp_mix_stereo_16bit_linear_filter
#endif
};

static mixer_set spline_mixers = {
	libxmp_mix_mono_8bit_spline,
	libxmp_mix_mono_16bit_spline,
	libxmp_mix_stereo_8bit_spline,
	libxmp_mix_stereo_16bit_spline,

#ifndef LIBXMP_CORE_DISABLE_IT
	libxmp_mix_mono_8bit_spline_filter,
	libxmp_mix_mono_16bit_spline_filter,
	libxmp_mix_stereo_8bit_spline_filter,
	libxmp_mix_stereo_16bit_spline_filter
#endif
};

#ifdef LIBXMP_PAULA_SIMULATOR
static mixer_set a500_mixers = {
	libxmp_mix_mono_a500,
	NULL,
	libxmp_mix_stereo_a500,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


static mixer_set a500led_mixers = {
	libxmp_mix_mono_a500_filter,
	NULL,
	libxmp_mix_stereo_a500_filter,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif


/* Downmix 32bit samples to 8bit, signed or unsigned, mono or stereo output */
static void downmix_int_8bit(char *dest, int32 *src, int num, int amp, int offs)
{
	int smp;
	int shift = DOWNMIX_SHIFT + 8 - amp;

	for (; num--; src++, dest++) {
		smp = *src >> shift;
		if (smp > LIM8_HI) {
			*dest = LIM8_HI;
		} else if (smp < LIM8_LO) {
			*dest = LIM8_LO;
		} else {
			*dest = smp;
		}

		if (offs) *dest += offs;
	}
}


/* Downmix 32bit samples to 16bit, signed or unsigned, mono or stereo output */
static void downmix_int_16bit(int16 *dest, int32 *src, int num, int amp, int offs)
{
	int smp;
	int shift = DOWNMIX_SHIFT - amp;

	for (; num--; src++, dest++) {
		smp = *src >> shift;
		if (smp > LIM16_HI) {
			*dest = LIM16_HI;
		} else if (smp < LIM16_LO) {
			*dest = LIM16_LO;
		} else {
			*dest = smp;
		}

		if (offs) *dest += offs;
	}
}

static void anticlick(struct mixer_voice *vi)
{
	vi->flags |= ANTICLICK;
	vi->old_vl = 0;
	vi->old_vr = 0;
}

/* Ok, it's messy, but it works :-) Hipolito */
static void do_anticlick(struct context_data *ctx, int voc, int32 *buf, int count)
{
	struct player_data *p = &ctx->p;
	struct mixer_data *s = &ctx->s;
	struct mixer_voice *vi = &p->virt.voice_array[voc];
	int smp_l, smp_r, max_x2;
	int discharge = s->ticksize >> ANTICLICK_SHIFT;

	smp_r = vi->sright;
	smp_l = vi->sleft;
	vi->sright = vi->sleft = 0;

	if (smp_l == 0 && smp_r == 0) {
		return;
	}

	if (buf == NULL) {
		buf = s->buf32;
		count = discharge;
	} else if (count > discharge) {
		count = discharge;
	}

	if (count <= 0) {
		return;
	}

	max_x2 = count * count;

	while (count--) {
		if (~s->format & XMP_FORMAT_MONO) {
			*buf++ += (count * (smp_r >> 10) / max_x2 * count) << 10;
		}

		*buf++ += (count * (smp_l >> 10) / max_x2 * count) << 10;
	}
}

static void set_sample_end(struct context_data *ctx, int voc, int end)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct mixer_voice *vi = &p->virt.voice_array[voc];
	struct channel_data *xc;

	if ((uint32)voc >= p->virt.maxvoc)
		return;

	xc = &p->xc_data[vi->chn];

	if (end) {
		SET_NOTE(NOTE_SAMPLE_END);
		if (HAS_QUIRK(QUIRK_RSTCHN)) {
			libxmp_virt_resetvoice(ctx, voc, 0);
		}
	} else {
		RESET_NOTE(NOTE_SAMPLE_END);
	}
}

static void adjust_voice_end(struct mixer_voice *vi, struct xmp_sample *xxs)
{
	if (xxs->flg & XMP_SAMPLE_LOOP) {
		if ((xxs->flg & XMP_SAMPLE_LOOP_FULL) && (~vi->flags & SAMPLE_LOOP)) {
			vi->end = xxs->len;
		} else {
			vi->end = xxs->lpe;
		}
	} else {
		vi->end = xxs->len;
	}
}

static void loop_reposition(struct context_data *ctx, struct mixer_voice *vi, struct xmp_sample *xxs)
{
#ifndef LIBXMP_CORE_DISABLE_IT
	struct module_data *m = &ctx->m;
#endif
	int loop_size = xxs->lpe - xxs->lps;

	/* Reposition for next loop */
	vi->pos -= loop_size;		/* forward loop */
	vi->end = xxs->lpe;
	vi->flags |= SAMPLE_LOOP;

	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		vi->end += loop_size;	/* unrolled loop */
		vi->pos -= loop_size;	/* forward loop */

#ifndef LIBXMP_CORE_DISABLE_IT
		/* OpenMPT Bidi-Loops.it: "In Impulse Tracker’s software mixer,
		 * ping-pong loops are shortened by one sample. 
		 */
		if (IS_PLAYER_MODE_IT()) {
			vi->end--;
			vi->pos++;
		}
#endif
	}
}


/* Prepare the mixer for the next tick */
void libxmp_mixer_prepare(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct mixer_data *s = &ctx->s;
	int bytelen;

	s->ticksize = (int)(s->freq * m->time_factor * m->rrate / p->bpm / 1000);

	bytelen = s->ticksize * sizeof(int);
	if (~s->format & XMP_FORMAT_MONO) {
		bytelen *= 2;
	}
	memset(s->buf32, 0, bytelen);
}
/* Fill the output buffer calling one of the handlers. The buffer contains
 * sound for one tick (a PAL frame or 1/50s for standard vblank-timed mods)
 */
void libxmp_mixer_softmixer(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct mixer_data *s = &ctx->s;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	struct xmp_sample *xxs;
	struct mixer_voice *vi;
	double step;
	int samples, size;
	int vol_l, vol_r, voc, usmp;
	int prev_l, prev_r = 0;
	int lps, lpe;
	int32 *buf_pos;
	void (*mix_fn)(struct mixer_voice *, int32 *, int, int, int, int, int, int, int);
	mixer_set *mixers;

	switch (s->interp) {
	case XMP_INTERP_NEAREST:
		mixers = (mixer_set *)&nearest_mixers;
		break;
	case XMP_INTERP_LINEAR:
		mixers = (mixer_set *)&linear_mixers;
		break;
	case XMP_INTERP_SPLINE:
		mixers = (mixer_set *)&spline_mixers;
		break;
	default:
		mixers = (mixer_set *)&linear_mixers;
	}

#ifdef LIBXMP_PAULA_SIMULATOR
	if (p->flags & XMP_FLAGS_A500) {
		if (IS_AMIGA_MOD()) {
			if (p->filter) {
				mixers = (mixer_set *)&a500led_mixers;
			} else {
				mixers = (mixer_set *)&a500_mixers;
			}
		}
	}
#endif

	libxmp_mixer_prepare(ctx);

	for (voc = 0; voc < p->virt.maxvoc; voc++) {
		int c5spd, rampsize, delta_l, delta_r;

		vi = &p->virt.voice_array[voc];

		if (vi->flags & ANTICLICK) {
			if (s->interp > XMP_INTERP_NEAREST) {
				do_anticlick(ctx, voc, NULL, 0);
			}
			vi->flags &= ~ANTICLICK;
		}

		if (vi->chn < 0) {
			continue;
		}

		if (vi->period < 1) {
			libxmp_virt_resetvoice(ctx, voc, 1);
			continue;
		}

		vi->pos0 = (int)vi->pos;

		buf_pos = s->buf32;
		if (vi->pan == PAN_SURROUND) {
			vol_r = vi->vol * 0x80;
			vol_l = -vi->vol * 0x80;
		} else {
			vol_r = vi->vol * (0x80 - vi->pan);
			vol_l = vi->vol * (0x80 + vi->pan);
		}

		if (vi->smp < mod->smp) {
			xxs = &mod->xxs[vi->smp];
			c5spd = (int) m->xtra[vi->smp].c5spd;
		} else {
			xxs = &ctx->smix.xxs[vi->smp - mod->smp];
			c5spd = m->c4rate;
		}

		step = C4_PERIOD * c5spd / s->freq / vi->period;

		if (step < 0.001) {	/* otherwise m5v-nwlf.it crashes */
			continue;
		}

#ifndef LIBXMP_CORE_DISABLE_IT
		if (xxs->flg & XMP_SAMPLE_SLOOP && vi->smp < mod->smp) {
			if (~vi->flags & VOICE_RELEASE) {
				if (vi->pos < m->xsmp[vi->smp].lpe) {
					xxs = &m->xsmp[vi->smp];
				}
			}
		}

		adjust_voice_end(vi, xxs);
#endif

		lps = xxs->lps;
		lpe = xxs->lpe;

		if (p->flags & XMP_FLAGS_FIXLOOP) {
			lps >>= 1;
		}

		if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
			vi->end += lpe - lps;

#ifndef LIBXMP_CORE_DISABLE_IT
			if (IS_PLAYER_MODE_IT()) {
				vi->end--;
			}
#endif
		}

		rampsize = s->ticksize >> ANTICLICK_SHIFT;
		delta_l = (vol_l - vi->old_vl) / rampsize;
		delta_r = (vol_r - vi->old_vr) / rampsize;

		usmp = 0;
		for (size = s->ticksize; size > 0; ) {
			int split_noloop = 0;

			if (p->xc_data[vi->chn].split) {
				split_noloop = 1;
			}

			/* How many samples we can write before the loop break
			 * or sample end... */
			if (vi->pos >= vi->end) {
				samples = 0;
				usmp = 1;
			} else {
				int s = (int) ceil(((double)vi->end - vi->pos) / step);
				/* ...inside the tick boundaries */
				if (s > size) {
					s = size;
				}

				samples = s;
				if (samples > 0) {
					usmp = 0;
				}
			}

			if (vi->vol) {
				int mix_size = samples;
				int mixer = vi->fidx & FIDX_FLAGMASK;

				if (~s->format & XMP_FORMAT_MONO) {
					mix_size *= 2;
				}

				/* For Hipolito's anticlick routine */
				if (samples > 0) {
					if (~s->format & XMP_FORMAT_MONO) {
						prev_r = buf_pos[mix_size - 2];
					}
					prev_l = buf_pos[mix_size - 1];
				} else {
					prev_r = prev_l = 0;
				}

#ifndef LIBXMP_CORE_DISABLE_IT
				/* See OpenMPT env-flt-max.it */
				if (vi->filter.cutoff >= 0xfe &&
                                    vi->filter.resonance == 0) {
					mixer &= ~FLAG_FILTER;
				}
#endif

				mix_fn = (*mixers)[mixer];

				/* Call the output handler */
				if (samples > 0 && vi->sptr != NULL) {
					int rsize = 0;

					if (rampsize > samples) {
						rampsize -= samples;
					} else {
						rsize = samples - rampsize;
						rampsize = 0;
					}

					if (delta_l == 0 && delta_r == 0) {
						/* no need to ramp */
						rsize = samples;
					}

					if (mix_fn != NULL) {
						mix_fn(vi, buf_pos, samples,
							vol_l >> 8, vol_r >> 8, (int) (step * (1 << SMIX_SHIFT)), rsize, delta_l, delta_r);
					}

					buf_pos += mix_size;
					vi->old_vl += samples * delta_l;
					vi->old_vr += samples * delta_r;


					/* For Hipolito's anticlick routine */
					if (~s->format & XMP_FORMAT_MONO) {
						vi->sright = buf_pos[-2] - prev_r;
					}
					vi->sleft = buf_pos[-1] - prev_l;
				}
			}

			vi->pos += step * samples;

			/* No more samples in this tick */
			size -= samples + usmp;
			if (size <= 0) {
				if (xxs->flg & XMP_SAMPLE_LOOP) {
					if (vi->pos + step > vi->end) {
						vi->pos += step;
						loop_reposition(ctx, vi, xxs);
					}
				}
				continue;
			}

			/* First sample loop run */
			if ((~xxs->flg & XMP_SAMPLE_LOOP) || split_noloop) {
				do_anticlick(ctx, voc, buf_pos, size);
				set_sample_end(ctx, voc, 1);
				size = 0;
				continue;
			}

			loop_reposition(ctx, vi, xxs);
		}

		vi->old_vl = vol_l;
		vi->old_vr = vol_r;
	}

	/* Render final frame */

	size = s->ticksize;
	if (~s->format & XMP_FORMAT_MONO) {
		size *= 2;
	}

	if (size > XMP_MAX_FRAMESIZE) {
		size = XMP_MAX_FRAMESIZE;
	}

	if (s->format & XMP_FORMAT_8BIT) {
		downmix_int_8bit(s->buffer, s->buf32, size, s->amplify,
				s->format & XMP_FORMAT_UNSIGNED ? 0x80 : 0);
	} else {
		downmix_int_16bit((int16 *)s->buffer, s->buf32, size,s->amplify,
				s->format & XMP_FORMAT_UNSIGNED ? 0x8000 : 0);
	}

	s->dtright = s->dtleft = 0;
}

void libxmp_mixer_voicepos(struct context_data *ctx, int voc, double pos, int ac)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct mixer_voice *vi = &p->virt.voice_array[voc];
	struct xmp_sample *xxs;
	int lps;

	if (vi->smp < m->mod.smp) {
 		xxs = &m->mod.xxs[vi->smp];
	} else {
 		xxs = &ctx->smix.xxs[vi->smp - m->mod.smp];
	}

	if (xxs->flg & XMP_SAMPLE_SYNTH) {
		return;
	}

	vi->pos = pos;

	adjust_voice_end(vi, xxs);

	if (vi->pos >= vi->end) {
		if (xxs->flg & XMP_SAMPLE_LOOP) {
			vi->pos = xxs->lps;
		} else {
			vi->pos = xxs->len;
		}
	}

	lps = xxs->lps;
	if (p->flags & XMP_FLAGS_FIXLOOP) {
		lps >>= 1;
	}

	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		vi->end += (xxs->lpe - lps);

#ifndef LIBXMP_CORE_DISABLE_IT
		if (IS_PLAYER_MODE_IT()) {
			vi->end--;
		}
#endif
	}

	if (ac) {
		anticlick(vi);
	}
}

double libxmp_mixer_getvoicepos(struct context_data *ctx, int voc)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];
	struct xmp_sample *xxs;

	xxs = libxmp_get_sample(ctx, vi->smp);

	if (xxs->flg & XMP_SAMPLE_SYNTH) {
		return 0;
	}

	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		if (vi->pos >= xxs->lpe) {
			return xxs->lpe - (vi->pos - xxs->lpe) - 1;
		}
	}

	return vi->pos;
}

void libxmp_mixer_setpatch(struct context_data *ctx, int voc, int smp, int ac)
{
	struct player_data *p = &ctx->p;
#ifndef LIBXMP_CORE_DISABLE_IT
	struct module_data *m = &ctx->m;
#endif
	struct mixer_data *s = &ctx->s;
	struct mixer_voice *vi = &p->virt.voice_array[voc];
	struct xmp_sample *xxs;

	xxs = libxmp_get_sample(ctx, smp);

	vi->smp = smp;
	vi->vol = 0;
	vi->pan = 0;
	vi->flags &= ~SAMPLE_LOOP;

	vi->fidx = 0;

	if (~s->format & XMP_FORMAT_MONO) {
		vi->fidx |= FLAG_STEREO;
	}

	set_sample_end(ctx, voc, 0);

	/*mixer_setvol(ctx, voc, 0);*/

	vi->sptr = xxs->data;
	vi->fidx |= FLAG_ACTIVE;

#ifndef LIBXMP_CORE_DISABLE_IT
	if (HAS_QUIRK(QUIRK_FILTER) && s->dsp & XMP_DSP_LOWPASS) {
		vi->fidx |= FLAG_FILTER;
	}
#endif

	if (xxs->flg & XMP_SAMPLE_16BIT) {
		vi->fidx |= FLAG_16_BITS;
	}

	libxmp_mixer_voicepos(ctx, voc, 0, ac);
}

void libxmp_mixer_setnote(struct context_data *ctx, int voc, int note)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	/* FIXME: Workaround for crash on notes that are too high
	 *        see 6nations.it (+114 transposition on instrument 16)
	 */
	if (note > 149) {
		note = 149;
	}

	vi->note = note;
	vi->period = libxmp_note_to_period_mix(note, 0);

	anticlick(vi);
}

void libxmp_mixer_setperiod(struct context_data *ctx, int voc, double period)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	vi->period = period;
}

void libxmp_mixer_setvol(struct context_data *ctx, int voc, int vol)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	if (vol == 0) {
		anticlick(vi);
	}

	vi->vol = vol;
}

void libxmp_mixer_release(struct context_data *ctx, int voc, int rel)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	if (rel) {
		vi->flags |= VOICE_RELEASE;
	} else {
		vi->flags &= ~VOICE_RELEASE;
	}
}

void libxmp_mixer_seteffect(struct context_data *ctx, int voc, int type, int val)
{
#ifndef LIBXMP_CORE_DISABLE_IT
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	switch (type) {
	case DSP_EFFECT_CUTOFF:
		vi->filter.cutoff = val;
		break;
	case DSP_EFFECT_RESONANCE:
		vi->filter.resonance = val;
		break;
	case DSP_EFFECT_FILTER_A0:
		vi->filter.a0 = val;
		break;
	case DSP_EFFECT_FILTER_B0:
		vi->filter.b0 = val;
		break;
	case DSP_EFFECT_FILTER_B1:
		vi->filter.b1 = val;
		break;
	}
#endif
}

void libxmp_mixer_setpan(struct context_data *ctx, int voc, int pan)
{
	struct player_data *p = &ctx->p;
	struct mixer_voice *vi = &p->virt.voice_array[voc];

	vi->pan = pan;
}

int libxmp_mixer_numvoices(struct context_data *ctx, int num)
{
	struct mixer_data *s = &ctx->s;

	if (num > s->numvoc || num < 0) {
		return s->numvoc;
	} else {
		return num;
	}
}

int libxmp_mixer_on(struct context_data *ctx, int rate, int format, int c4rate)
{
	struct mixer_data *s = &ctx->s;

	s->buffer = (char *)calloc(2, XMP_MAX_FRAMESIZE);
	if (s->buffer == NULL)
		goto err;

	s->buf32 = (int32 *)calloc(sizeof(int32), XMP_MAX_FRAMESIZE);
	if (s->buf32 == NULL)
		goto err1;

	s->freq = rate;
	s->format = format;
	s->amplify = DEFAULT_AMPLIFY;
	s->mix = DEFAULT_MIX;
	/* s->pbase = C4_PERIOD * c4rate / s->freq; */
	s->interp = XMP_INTERP_LINEAR;	/* default interpolation type */
	s->dsp = XMP_DSP_LOWPASS;	/* enable filters by default */
	/* s->numvoc = SMIX_NUMVOC; */
	s->dtright = s->dtleft = 0;

	return 0;

    err1:
	free(s->buffer);
    err:
	return -1;
}

void libxmp_mixer_off(struct context_data *ctx)
{
	struct mixer_data *s = &ctx->s;

	free(s->buffer);
	free(s->buf32);
	s->buf32 = NULL;
	s->buffer = NULL;
}
