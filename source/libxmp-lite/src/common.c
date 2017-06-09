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

#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#ifdef __WATCOMC__
#include <direct.h>
#elif !defined(_WIN32)
#include <dirent.h>
#endif

#include "xmp.h"
#include "common.h"
#include "period.h"
#include "loader.h"

int libxmp_init_instrument(struct module_data *m)
{
	struct xmp_module *mod = &m->mod;

	if (mod->ins > 0) {
		mod->xxi = (struct xmp_instrument *)calloc(sizeof (struct xmp_instrument), mod->ins);
		if (mod->xxi == NULL)
			return -1;
	}

	if (mod->smp > 0) {
		int i;

		mod->xxs = (struct xmp_sample *)calloc(sizeof (struct xmp_sample), mod->smp);
		if (mod->xxs == NULL)
			return -1;
		m->xtra = (struct extra_sample_data *)calloc(sizeof (struct extra_sample_data), mod->smp);
		if (m->xtra == NULL)
			return -1;

		for (i = 0; i < mod->smp; i++) {
			m->xtra[i].c5spd = m->c4rate;
		}
	}

	return 0;
}

int libxmp_alloc_subinstrument(struct xmp_module *mod, int i, int num)
{
	if (num == 0)
		return 0;

	mod->xxi[i].sub = (struct xmp_subinstrument *)calloc(sizeof (struct xmp_subinstrument), num);
	if (mod->xxi[i].sub == NULL)
		return -1;

	return 0;
}

int libxmp_init_pattern(struct xmp_module *mod)
{
	mod->xxt = (struct xmp_track **)calloc(sizeof (struct xmp_track *), mod->trk);
	if (mod->xxt == NULL)
		return -1;

	mod->xxp = (struct xmp_pattern **)calloc(sizeof (struct xmp_pattern *), mod->pat);
	if (mod->xxp == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_pattern(struct xmp_module *mod, int num)
{
	/* Sanity check */
	if (num < 0 || num >= mod->pat || mod->xxp[num] != NULL)
		return -1;

	mod->xxp[num] = (struct xmp_pattern *)calloc(1, sizeof (struct xmp_pattern) +
        				sizeof (int) * (mod->chn - 1));
	if (mod->xxp[num] == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_track(struct xmp_module *mod, int num, int rows)
{
	/* Sanity check */
	if (num < 0 || num >= mod->trk || mod->xxt[num] != NULL || rows <= 0)
		return -1;

	mod->xxt[num] = (struct xmp_track *)calloc(sizeof (struct xmp_track) +
			       sizeof (struct xmp_event) * (rows - 1), 1);
	if (mod->xxt[num] == NULL)
		return -1;

	mod->xxt[num]->rows = rows;

	return 0;
}

int libxmp_alloc_tracks_in_pattern(struct xmp_module *mod, int num)
{
	int i;

	D_(D_INFO "Alloc %d tracks of %d rows", mod->chn, mod->xxp[num]->rows);
	for (i = 0; i < mod->chn; i++) {
		int t = num * mod->chn + i;
		int rows = mod->xxp[num]->rows;

		if (libxmp_alloc_track(mod, t, rows) < 0)
			return -1;

		mod->xxp[num]->index[i] = t;
	}

	return 0;
}

int libxmp_alloc_pattern_tracks(struct xmp_module *mod, int num, int rows)
{
	/* Sanity check */
	if (rows < 0 || rows > 256)
		return -1;

	if (libxmp_alloc_pattern(mod, num) < 0)
		return -1;

	mod->xxp[num]->rows = rows;

	if (libxmp_alloc_tracks_in_pattern(mod, num) < 0)
		return -1;

	return 0;
}

/* Sample number adjustment by Vitamin/CAIG */
struct xmp_sample *libxmp_realloc_samples(struct xmp_sample *buf, int *size, int new_size)
{
	buf = (struct xmp_sample *)realloc(buf, sizeof (struct xmp_sample) * new_size);
	if (buf == NULL)
		return NULL;
	if (new_size > *size)
		memset(buf + *size, 0, sizeof (struct xmp_sample) * (new_size - *size));
	*size = new_size;

	return buf;
}

char *libxmp_instrument_name(struct xmp_module *mod, int i, uint8 *r, int n)
{
	CLAMP(n, 0, 31);

	return libxmp_copy_adjust(mod->xxi[i].name, r, n);
}

char *libxmp_copy_adjust(char *s, uint8 *r, int n)
{
	int i;

	memset(s, 0, n + 1);
	strncpy(s, (char *)r, n);

	for (i = 0; s[i] && i < n; i++) {
		if (!isprint((int)s[i]) || ((uint8)s[i] > 127))
			s[i] = '.';
	}

	while (*s && (s[strlen(s) - 1] == ' '))
		s[strlen(s) - 1] = 0;

	return s;
}

void libxmp_read_title(HIO_HANDLE *f, char *t, int s)
{
	uint8 buf[XMP_NAME_SIZE];

	if (t == NULL)
		return;

	if (s >= XMP_NAME_SIZE)
		s = XMP_NAME_SIZE -1;

	memset(t, 0, s + 1);

	hio_read(buf, 1, s, f);		/* coverity[check_return] */
	buf[s] = 0;
	libxmp_copy_adjust(t, buf, s);
}

#ifndef LIBXMP_CORE_PLAYER

int libxmp_test_name(uint8 *s, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if (s[i] > 0x7f)
			return -1;
		/* ACS_Team2.mod has a backspace in instrument name */
		if (s[i] > 0 && s[i] < 32 && s[i] != 0x08)
			return -1;
	}

	return 0;
}

/*
 * Honor Noisetracker effects:
 *
 *  0 - arpeggio
 *  1 - portamento up
 *  2 - portamento down
 *  3 - Tone-portamento
 *  4 - Vibrato
 *  A - Slide volume
 *  B - Position jump
 *  C - Set volume
 *  D - Pattern break
 *  E - Set filter (keep the led off, please!)
 *  F - Set speed (now up to $1F)
 *
 * Pex Tufvesson's notes from http://www.livet.se/mahoney/:
 *
 * Note that some of the modules will have bugs in the playback with all
 * known PC module players. This is due to that in many demos where I synced
 * events in the demo with the music, I used commands that these newer PC
 * module players erroneously interpret as "newer-version-trackers commands".
 * Which they aren't.
 */
void libxmp_decode_noisetracker_event(struct xmp_event *event, uint8 *mod_event)
{
	int fxt;

	memset(event, 0, sizeof (struct xmp_event));
	event->note = libxmp_period_to_note((LSN(mod_event[0]) << 8) + mod_event[1]);
	event->ins = ((MSN(mod_event[0]) << 4) | MSN(mod_event[2]));
	fxt = LSN(mod_event[2]);

	if (fxt <= 0x06 || (fxt >= 0x0a && fxt != 0x0e)) {
		event->fxt = fxt;
		event->fxp = mod_event[3];
	}

	libxmp_disable_continue_fx(event);
}
#endif

void libxmp_decode_protracker_event(struct xmp_event *event, uint8 *mod_event)
{
	int fxt = LSN(mod_event[2]);

	memset(event, 0, sizeof (struct xmp_event));
	event->note = libxmp_period_to_note((LSN(mod_event[0]) << 8) + mod_event[1]);
	event->ins = ((MSN(mod_event[0]) << 4) | MSN(mod_event[2]));

	if (fxt != 0x08) {
		event->fxt = fxt;
		event->fxp = mod_event[3];
	}

	libxmp_disable_continue_fx(event);
}

void libxmp_disable_continue_fx(struct xmp_event *event)
{
	if (event->fxp == 0) {
		switch (event->fxt) {
		case 0x05:
			event->fxt = 0x03;
			break;
		case 0x06:
			event->fxt = 0x04;
			break;
		case 0x01:
		case 0x02:
		case 0x0a:
			event->fxt = 0x00;
		}
	} else if (event->fxt == 0x0e) {
		if (event->fxp == 0xa0 || event->fxp == 0xb0) {
			event->fxt = event->fxp = 0;
		}
	}
}

#ifndef LIBXMP_CORE_PLAYER
#ifndef WIN32

/* Given a directory, see if file exists there, ignoring case */

int libxmp_check_filename_case(char *dir, char *name, char *new_name, int size)
{
	int found = 0;
	DIR *dirfd;
	struct dirent *d;

	dirfd = opendir(dir);
	if (dirfd == NULL)
		return 0;
 
	while ((d = readdir(dirfd))) {
		if (!strcasecmp(d->d_name, name)) {
			found = 1;
			break;
		}
	}

	if (found)
		strncpy(new_name, d->d_name, size);

	closedir(dirfd);

	return found;
}

#else

/* FIXME: implement functionality for Win32 */

int libxmp_check_filename_case(char *dir, char *name, char *new_name, int size)
{
	return 0;
}

#endif

void libxmp_get_instrument_path(struct module_data *m, char *path, int size)
{
	if (m->instrument_path) {
		strncpy(path, m->instrument_path, size);
	} else if (getenv("XMP_INSTRUMENT_PATH")) {
		strncpy(path, getenv("XMP_INSTRUMENT_PATH"), size);
	} else {
		strncpy(path, ".", size);
	}
}
#endif /* LIBXMP_CORE_PLAYER */

void libxmp_set_type(struct module_data *m, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	vsnprintf(m->mod.type, XMP_NAME_SIZE, fmt, ap);
	va_end(ap);
}
