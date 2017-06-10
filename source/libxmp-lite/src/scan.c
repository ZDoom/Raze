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
 * Sun, 31 May 1998 17:50:02 -0600
 * Reported by ToyKeeper <scriven@CS.ColoState.EDU>:
 * For loop-prevention, I know a way to do it which lets most songs play
 * fine once through even if they have backward-jumps. Just keep a small
 * array (256 bytes, or even bits) of flags, each entry determining if a
 * pattern in the song order has been played. If you get to an entry which
 * is already non-zero, skip to the next song (assuming looping is off).
 */

/*
 * Tue, 6 Oct 1998 21:23:17 +0200 (CEST)
 * Reported by John v/d Kamp <blade_@dds.nl>:
 * scan.c was hanging when it jumps to an invalid restart value.
 * (Fixed by hipolito)
 */


#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "effects.h"
#include "mixer.h"

#define S3M_END		0xff
#define S3M_SKIP	0xfe


static int scan_module(struct context_data *ctx, int ep, int chain)
{
    struct player_data *p = &ctx->p;
    struct module_data *m = &ctx->m;
    struct xmp_module *mod = &m->mod;
    int parm, gvol_memory, f1, f2, p1, p2, ord, ord2;
    int row, last_row, break_row, row_count;
    int gvl, bpm, speed, base_time, chn;
    int frame_count;
    double time, start_time;
    int loop_chn, loop_num, inside_loop;
    int pdelay = 0;
    int loop_count[XMP_MAX_CHANNELS];
    int loop_row[XMP_MAX_CHANNELS];
    struct xmp_event* event;
    int i, pat;
    int has_marker;
    struct ord_data *info;
#ifndef LIBXMP_CORE_PLAYER
    int st26_speed;
#endif

    if (mod->len == 0)
	return 0;

    for (i = 0; i < mod->len; i++) {
	int pat = mod->xxo[i];
	memset(m->scan_cnt[i], 0, pat >= mod->pat ? 1 :
			mod->xxp[pat]->rows ? mod->xxp[pat]->rows : 1);
    }

    for (i = 0; i < mod->chn; i++) {
	loop_count[i] = 0;
	loop_row[i] = -1;
    }
    loop_num = 0;
    loop_chn = -1;

    gvl = mod->gvl;
    bpm = mod->bpm;

    speed = mod->spd;
    base_time = (int) m->rrate;
#ifndef LIBXMP_CORE_PLAYER
    st26_speed = 0;
#endif

    has_marker = HAS_QUIRK(QUIRK_MARKER);

    /* By erlk ozlr <erlk.ozlr@gmail.com>
     *
     * xmp doesn't handle really properly the "start" option (-s for the
     * command-line) for DeusEx's .umx files. These .umx files contain
     * several loop "tracks" that never join together. That's how they put
     * multiple musics on each level with a file per level. Each "track"
     * starts at the same order in all files. The problem is that xmp starts
     * decoding the module at order 0 and not at the order specified with
     * the start option. If we have a module that does "0 -> 2 -> 0 -> ...",
     * we cannot play order 1, even with the supposed right option.
     *
     * was: ord2 = ord = -1;
     *
     * CM: Fixed by using different "sequences" for each loop or subsong.
     *     Each sequence has its entry point. Sequences don't overlap.
     */
    ord2 = -1;
    ord = ep - 1;

    gvol_memory = break_row = row_count = frame_count = 0;
    start_time = time = 0.0;
    inside_loop = 0;

    while (42) {
	if ((uint32)++ord >= mod->len) {
	    if (mod->rst > mod->len || mod->xxo[mod->rst] >= mod->pat) {
		ord = ep;
	    } else {
		if (libxmp_get_sequence(ctx, mod->rst) == chain) {
	            ord = mod->rst;
		} else {
		    ord = ep;
	        }
	    }
	   
	    pat = mod->xxo[ord];
	    if (has_marker && pat == S3M_END) {
		break;
	    }
	} 

	pat = mod->xxo[ord];
	info = &m->xxo_info[ord];

	/* Allow more complex order reuse only in main sequence */
	if (ep != 0 && p->sequence_control[ord] != 0xff) {
	    break;
	}
	p->sequence_control[ord] = chain;

	/* All invalid patterns skipped, only S3M_END aborts replay */
	if (pat >= mod->pat) {
	    if (has_marker && pat == S3M_END) {
		ord = mod->len;
	        continue;
	    }
	    continue;
	}

        if (break_row >= mod->xxp[pat]->rows) {
            break_row = 0;
        }

        /* Loops can cross pattern boundaries, so check if we're not looping */
        if (m->scan_cnt[ord][break_row] && !inside_loop) {
            break;
        }

        /* Don't update pattern information if we're inside a loop, otherwise
         * a loop containing e.g. a global volume fade can make the pattern
         * start with the wrong volume.
         */
        if (!inside_loop && info->gvl < 0) {
            info->gvl = gvl;
            info->bpm = bpm;
            info->speed = speed;
            info->time = (int) (time + m->time_factor * frame_count * base_time / bpm);
#ifndef LIBXMP_CORE_PLAYER
            info->st26_speed = st26_speed;
#endif
        }

	if (info->start_row == 0 && ord != 0) {
	    if (ord == ep) {
		start_time = time + m->time_factor * frame_count * base_time / bpm;
	    }

	    info->start_row = break_row;
	}

	last_row = mod->xxp[pat]->rows;
	for (row = break_row, break_row = 0; row < last_row; row++, row_count++) {
	    /* Prevent crashes caused by large softmixer frames */
	    if (bpm < XMP_MIN_BPM) {
	        bpm = XMP_MIN_BPM;
	    }

	    /* Date: Sat, 8 Sep 2007 04:01:06 +0200
	     * Reported by Zbigniew Luszpinski <zbiggy@o2.pl>
	     * The scan routine falls into infinite looping and doesn't let
	     * xmp play jos-dr4k.xm.
	     * Claudio's workaround: we'll break infinite loops here.
	     *
	     * Date: Oct 27, 2007 8:05 PM
	     * From: Adric Riedel <adric.riedel@gmail.com>
	     * Jesper Kyd: Global Trash 3.mod (the 'Hardwired' theme) only
	     * plays the first 4:41 of what should be a 10 minute piece.
	     * (...) it dies at the end of position 2F
	     */

	    if (row_count > 512)  /* was 255, but Global trash goes to 318 */
		goto end_module;

	    if (!loop_num && m->scan_cnt[ord][row]) {
		row_count--;
		goto end_module;
	    }
	    m->scan_cnt[ord][row]++;

	    pdelay = 0;

	    for (chn = 0; chn < mod->chn; chn++) {
		if (row >= mod->xxt[mod->xxp[pat]->index[chn]]->rows)
		    continue;

		event = &EVENT(mod->xxo[ord], chn, row);

		f1 = event->fxt;
		p1 = event->fxp;
		f2 = event->f2t;
		p2 = event->f2p;

		if (f1 == FX_GLOBALVOL || f2 == FX_GLOBALVOL) {
		    gvl = (f1 == FX_GLOBALVOL) ? p1 : p2;
		    gvl = gvl > m->gvolbase ? m->gvolbase : gvl < 0 ? 0 : gvl;
		}

		/* Process fine global volume slide */
		if (f1 == FX_GVOL_SLIDE || f2 == FX_GVOL_SLIDE) {
		    int h, l;
		    parm = (f1 == FX_GVOL_SLIDE) ? p1 : p2;

		process_gvol:
                    if (parm) {
			gvol_memory = parm;
                        h = MSN(parm);
                        l = LSN(parm);

		        if (HAS_QUIRK(QUIRK_FINEFX)) {
                            if (l == 0xf && h != 0) {
				gvl += h;
			    } else if (h == 0xf && l != 0) {
				gvl -= l;
			    } else {
		                if (m->quirk & QUIRK_VSALL) {
                                    gvl += (h - l) * speed;
				} else {
                                    gvl += (h - l) * (speed - 1);
				}
			    }
			} else {
		            if (m->quirk & QUIRK_VSALL) {
                                gvl += (h - l) * speed;
			    } else {
                                gvl += (h - l) * (speed - 1);
			    }
			}
		    } else {
                        if ((parm = gvol_memory) != 0)
			    goto process_gvol;
		    }
		}

		if ((f1 == FX_SPEED && p1) || (f2 == FX_SPEED && p2)) {
		    parm = (f1 == FX_SPEED) ? p1 : p2;
		    frame_count += row_count * speed;
		    row_count = 0;
		    if (parm) {
			if (HAS_QUIRK(QUIRK_NOBPM) || p->flags & XMP_FLAGS_VBLANK || parm < 0x20) {
			    if (parm > 0) {
			        speed = parm;
#ifndef LIBXMP_CORE_PLAYER
			        st26_speed = 0;
#endif
                            }
			} else {
			    time += m->time_factor * frame_count * base_time / bpm;
			    frame_count = 0;
			    bpm = parm;
			}
		    }
		}

#ifndef LIBXMP_CORE_PLAYER
		if (f1 == FX_SPEED_CP) {
		    f1 = FX_S3M_SPEED;
		}
		if (f2 == FX_SPEED_CP) {
		    f2 = FX_S3M_SPEED;
		}

		/* ST2.6 speed processing */

		if (f1 == FX_ICE_SPEED && p1) {
		    if (LSN(p1)) {
		        st26_speed = (MSN(p1) << 8) | LSN(p1);
		    } else {
			st26_speed = MSN(p1);
		    }
		}
#endif

		if ((f1 == FX_S3M_SPEED && p1) || (f2 == FX_S3M_SPEED && p2)) {
		    parm = (f1 == FX_S3M_SPEED) ? p1 : p2;
		    if (parm > 0) {
		        frame_count += row_count * speed;
		        row_count  = 0;
		        speed = parm;
#ifndef LIBXMP_CORE_PLAYER
		        st26_speed = 0;
#endif
		    }
		}

		if ((f1 == FX_S3M_BPM && p1) || (f2 == FX_S3M_BPM && p2)) {
		    parm = (f1 == FX_S3M_BPM) ? p1 : p2;
		    if (parm >= 0x20) {
			frame_count += row_count * speed;
			row_count = 0;
			time += m->time_factor * frame_count * base_time / bpm;
			frame_count = 0;
			bpm = parm;
		    }
		}

#ifndef LIBXMP_CORE_DISABLE_IT
		if ((f1 == FX_IT_BPM && p1) || (f2 == FX_IT_BPM && p2)) {
		    parm = (f1 == FX_IT_BPM) ? p1 : p2;
		    frame_count += row_count * speed;
		    row_count = 0;
		    time += m->time_factor * frame_count * base_time / bpm;
		    frame_count = 0;

		    if (MSN(parm) == 0) {
		        time += m->time_factor * base_time / bpm;
			for (i = 1; i < speed; i++) {
			    bpm -= LSN(parm);
			    if (bpm < 0x20)
				bpm = 0x20;
		            time += m->time_factor * base_time / bpm;
			}

			/* remove one row at final bpm */
			time -= m->time_factor * speed * base_time / bpm;

		    } else if (MSN(parm) == 1) {
		        time += m->time_factor * base_time / bpm;
			for (i = 1; i < speed; i++) {
			    bpm += LSN(parm);
			    if (bpm > 0xff)
				bpm = 0xff;
		            time += m->time_factor * base_time / bpm;
			}

			/* remove one row at final bpm */
			time -= m->time_factor * speed * base_time / bpm;

		    } else {
			bpm = parm;
		    }
		}

		if (f1 == FX_IT_ROWDELAY) {
	    		m->scan_cnt[ord][row] += p1 & 0x0f;
			frame_count += (p1 & 0x0f) * speed;
		}

		if (f1 == FX_IT_BREAK) {
		    break_row = p1;
		    last_row = 0;
		}
#endif

		if (f1 == FX_JUMP || f2 == FX_JUMP) {
		    ord2 = (f1 == FX_JUMP) ? p1 : p2;
		    break_row = 0;
		    last_row = 0;

		    /* prevent infinite loop, see OpenMPT PatLoop-Various.xm */
		    inside_loop = 0;
		}

		if (f1 == FX_BREAK || f2 == FX_BREAK) {
		    parm = (f1 == FX_BREAK) ? p1 : p2;
		    break_row = 10 * MSN(parm) + LSN(parm);
		    last_row = 0;
		}

		if (f1 == FX_EXTENDED || f2 == FX_EXTENDED) {
		    parm = (f1 == FX_EXTENDED) ? p1 : p2;

		    if ((parm >> 4) == EX_PATT_DELAY) {
			if (m->read_event_type != READ_EVENT_ST3 || !pdelay) {
			    pdelay = parm & 0x0f;
			    frame_count += pdelay * speed;
                        }
		    }

		    if ((parm >> 4) == EX_PATTERN_LOOP) {
			if (parm &= 0x0f) {
			    /* Loop end */
			    if (loop_count[chn]) {
				if (--loop_count[chn]) {
				    /* next iteraction */
				    loop_chn = chn;
				} else {
				    /* finish looping */
				    loop_num--;
				    inside_loop = 0;
				    if (m->quirk & QUIRK_S3MLOOP)
					loop_row[chn] = row;
				}
			    } else {
				loop_count[chn] = parm;
				loop_chn = chn;
				loop_num++;
			    }
			} else { 
			    /* Loop start */
			    loop_row[chn] = row - 1;
			    inside_loop = 1;
			    if (HAS_QUIRK(QUIRK_FT2BUGS))
				break_row = row;
			}
		    }
		}
	    }

	    if (loop_chn >= 0) {
		row = loop_row[loop_chn];
		loop_chn = -1;
	    }

#ifndef LIBXMP_CORE_PLAYER
	    if (st26_speed) {
	        frame_count += row_count * speed;
	        row_count  = 0;
		if (st26_speed & 0x10000) {
			speed = (st26_speed & 0xff00) >> 8;
		} else {
			speed = st26_speed & 0xff;
		}
		st26_speed ^= 0x10000;
	    }
#endif
	}

	if (break_row && pdelay) {
	    break_row++;
	}

	if (ord2 >= 0) {
	    ord = ord2 - 1;
	    ord2 = -1;
	}

	frame_count += row_count * speed;
	row_count = 0;
    }
    row = break_row;

end_module:

    /* Sanity check */
    {
        pat = mod->xxo[ord];
        if (pat >= mod->pat || row >= mod->xxp[pat]->rows) {
            row = 0;
        }
    }

    p->scan[chain].num = m->scan_cnt[ord][row];
    p->scan[chain].row = row;
    p->scan[chain].ord = ord;

    time -= start_time;
    frame_count += row_count * speed;

    return (int) (time + m->time_factor * frame_count * base_time / bpm);
}

int libxmp_get_sequence(struct context_data *ctx, int ord)
{
	struct player_data *p = &ctx->p;
	return p->sequence_control[ord];
}

int libxmp_scan_sequences(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i, ep;
	int seq;
	unsigned char temp_ep[XMP_MAX_MOD_LENGTH];

	/* Initialize order data to prevent overwrite when a position is used
	 * multiple times at different starting points (see janosik.xm).
	 */
	for (i = 0; i < XMP_MAX_MOD_LENGTH; i++) {
		m->xxo_info[i].gvl = -1;
	}

	ep = 0;
	memset(p->sequence_control, 0xff, XMP_MAX_MOD_LENGTH);
	temp_ep[0] = 0;
	p->scan[0].time = scan_module(ctx, ep, 0);
	seq = 1;

 	while (1) { 
		/* Scan song starting at given entry point */
		/* Check if any patterns left */
		for (i = 0; i < mod->len; i++) {
			if (p->sequence_control[i] == 0xff) {
				break;
			}
		}
		if (i != mod->len && seq < MAX_SEQUENCES) {
			/* New entry point */
			ep = i;
			temp_ep[seq] = ep;
			p->scan[seq].time = scan_module(ctx, ep, seq);
			if (p->scan[seq].time > 0)
				seq++;
		} else {
			break;
		}
	}

	m->num_sequences = seq;

	/* Now place entry points in the public accessible array */
	for (i = 0; i < m->num_sequences; i++) {
		m->seq_data[i].entry_point = temp_ep[i];
		m->seq_data[i].duration = p->scan[i].time;
	}


	return 0;
}
