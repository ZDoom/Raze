#ifndef LIBXMP_COMMON_H
#define LIBXMP_COMMON_H

#ifdef __AROS__
#define __AMIGA__
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "xmp.h"

#if (defined(__GNUC__) || defined(__clang__)) && defined(XMP_SYM_VISIBILITY)
#if !defined(WIN32) && !defined(__ANDROID__) && !defined(__APPLE__) && !defined(__AMIGA__) && !defined(__MSDOS__) && !defined(B_BEOS_VERSION) && !defined(__ATHEOS__) && !defined(EMSCRIPTEN) && !defined(__MINT__)
#define USE_VERSIONED_SYMBOLS
#endif
#endif

/* AmigaOS fixes by Chris Young <cdyoung@ntlworld.com>, Nov 25, 2007
 */
#if defined B_BEOS_VERSION
#  include <SupportDefs.h>
#elif defined __amigaos4__
#  include <exec/types.h>
#else
typedef signed char int8;
typedef signed short int int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
#endif

#ifdef _MSC_VER				/* MSVC++6.0 has no long long */
typedef signed __int64 int64;
typedef unsigned __int64 uint64;
#elif !defined B_BEOS_VERSION		/* BeOS has its own int64 definition */
typedef unsigned long long uint64;
typedef signed long long int64;
#endif

#ifndef LIBXMP_CORE_PLAYER
#define LIBXMP_PAULA_SIMULATOR
#endif

/* Constants */
#define PAL_RATE	250.0		/* 1 / (50Hz * 80us)		  */
#define NTSC_RATE	208.0		/* 1 / (60Hz * 80us)		  */
#define C4_PAL_RATE	8287		/* 7093789.2 / period (C4) * 2	  */
#define C4_NTSC_RATE	8363		/* 7159090.5 / period (C4) * 2	  */

/* [Amiga] PAL color carrier frequency (PCCF) = 4.43361825 MHz */
/* [Amiga] CPU clock = 1.6 * PCCF = 7.0937892 MHz */

#define DEFAULT_AMPLIFY	1
#define DEFAULT_MIX	100

#define MSN(x)		(((x)&0xf0)>>4)
#define LSN(x)		((x)&0x0f)
#define SET_FLAG(a,b)	((a)|=(b))
#define RESET_FLAG(a,b)	((a)&=~(b))
#define TEST_FLAG(a,b)	!!((a)&(b))

#define CLAMP(x,a,b) do { \
    if ((x) < (a)) (x) = (a); \
    else if ((x) > (b)) (x) = (b); \
} while (0)
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define TRACK_NUM(a,c)	m->mod.xxp[a]->index[c]
#define EVENT(a,c,r)	m->mod.xxt[TRACK_NUM((a),(c))]->event[r]

#ifdef _MSC_VER
#define D_CRIT "  Error: "
#define D_WARN "Warning: "
#define D_INFO "   Info: "
#ifndef CLIB_DECL
#define CLIB_DECL
#endif
#ifdef DEBUG
#ifndef ATTR_PRINTF
#define ATTR_PRINTF(x,y)
#endif
void CLIB_DECL D_(const char *text, ...) ATTR_PRINTF(1,2);
#else
// VS prior to VC7.1 does not support variadic macros. VC8.0 does not optimize unused parameters passing
#if _MSC_VER < 1400
void __inline CLIB_DECL D_(const char *text, ...) { do {} while (0); }
#else
#define D_(args, ...) do {} while (0)
#endif
#endif

#elif defined __ANDROID__

#ifdef DEBUG
#include <android/log.h>
#define D_CRIT "  Error: "
#define D_WARN "Warning: "
#define D_INFO "   Info: "
#define D_(args...) do { \
	__android_log_print(ANDROID_LOG_DEBUG, "libxmp", args); \
	} while (0)
#else
#define D_(args...) do {} while (0)
#endif

#elif defined(__WATCOMC__)
#ifdef DEBUG
#define D_INFO "\x1b[33m"
#define D_CRIT "\x1b[31m"
#define D_WARN "\x1b[36m"
#define D_(...) do { \
	printf("\x1b[33m%s \x1b[37m[%s:%d] " D_INFO, __FUNCTION__, \
		__FILE__, __LINE__); printf (__VA_ARGS__); printf ("\x1b[0m\n"); \
	} while (0)
#else
#define D_(...) do {} while (0)
#endif

#else

#ifdef DEBUG
#define D_INFO "\x1b[33m"
#define D_CRIT "\x1b[31m"
#define D_WARN "\x1b[36m"
#define D_(args...) do { \
	printf("\x1b[33m%s \x1b[37m[%s:%d] " D_INFO, __FUNCTION__, \
		__FILE__, __LINE__); printf (args); printf ("\x1b[0m\n"); \
	} while (0)
#else
#define D_(args...) do {} while (0)
#endif

#endif	/* !_MSC_VER */

#ifdef _MSC_VER
#define dup _dup
#define fileno _fileno
#define strnicmp _strnicmp
#define strdup _strdup
#define fdopen _fdopen
#define open _open
#define close _close
#define unlink _unlink
#endif
#if defined(_WIN32) || defined(__WATCOMC__) /* in win32.c */
int libxmp_vsnprintf(char *, size_t, const char *, va_list);
int libxmp_snprintf (char *, size_t, const char *, ...);
#define snprintf  libxmp_snprintf
#define vsnprintf libxmp_vsnprintf
#endif

/* Quirks */
#define QUIRK_S3MLOOP	(1 << 0)	/* S3M loop mode */
#define QUIRK_ENVFADE	(1 << 1)	/* Fade at end of envelope */
#define QUIRK_PROTRACK	(1 << 2)	/* Use Protracker-specific quirks */
#define QUIRK_ST3BUGS	(1 << 4)	/* Scream Tracker 3 bug compatibility */
#define QUIRK_FINEFX	(1 << 5)	/* Enable 0xf/0xe for fine effects */
#define QUIRK_VSALL	(1 << 6)	/* Volume slides in all frames */
#define QUIRK_PBALL	(1 << 7)	/* Pitch bending in all frames */
#define QUIRK_PERPAT	(1 << 8)	/* Cancel persistent fx at pat start */
#define QUIRK_VOLPDN	(1 << 9)	/* Set priority to volume slide down */
#define QUIRK_UNISLD	(1 << 10)	/* Unified pitch slide/portamento */
#define QUIRK_ITVPOR	(1 << 11)	/* Disable fine bends in IT vol fx */
#define QUIRK_FTMOD	(1 << 12)	/* Flag for multichannel mods */
/*#define QUIRK_MODRNG	(1 << 13)*/	/* Limit periods to MOD range */
#define QUIRK_INSVOL	(1 << 14)	/* Use instrument volume */
#define QUIRK_VIRTUAL	(1 << 15)	/* Enable virtual channels */
#define QUIRK_FILTER	(1 << 16)	/* Enable filter */
#define QUIRK_IGSTPOR	(1 << 17)	/* Ignore stray tone portamento */
#define QUIRK_KEYOFF	(1 << 18)	/* Keyoff doesn't reset fadeout */
#define QUIRK_VIBHALF	(1 << 19)	/* Vibrato is half as deep */
#define QUIRK_VIBALL	(1 << 20)	/* Vibrato in all frames */
#define QUIRK_VIBINV	(1 << 21)	/* Vibrato has inverse waveform */
#define QUIRK_PRENV	(1 << 22)	/* Portamento resets envelope & fade */
#define QUIRK_ITOLDFX	(1 << 23)	/* IT old effects mode */
#define QUIRK_S3MRTG	(1 << 24)	/* S3M-style retrig when count == 0 */
#define QUIRK_RTDELAY	(1 << 25)	/* Delay effect retrigs instrument */
#define QUIRK_FT2BUGS	(1 << 26)	/* FT2 bug compatibility */
#define QUIRK_MARKER	(1 << 27)	/* Patterns 0xfe and 0xff reserved */
#define QUIRK_NOBPM	(1 << 28)	/* Adjust speed only, no BPM */
#define QUIRK_ARPMEM	(1 << 29)	/* Arpeggio has memory (S3M_ARPEGGIO) */
#define QUIRK_RSTCHN	(1 << 30)	/* Reset channel on sample end */

#define HAS_QUIRK(x)	(m->quirk & (x))


/* Format quirks */
#define QUIRKS_ST3		(QUIRK_S3MLOOP | QUIRK_VOLPDN | QUIRK_FINEFX | \
				 QUIRK_S3MRTG  | QUIRK_MARKER | QUIRK_RSTCHN )
#define QUIRKS_FT2		(QUIRK_RTDELAY | QUIRK_FINEFX )
#define QUIRKS_IT		(QUIRK_S3MLOOP | QUIRK_FINEFX | QUIRK_VIBALL | \
				 QUIRK_ENVFADE | QUIRK_ITVPOR | QUIRK_KEYOFF | \
				 QUIRK_VIRTUAL | QUIRK_FILTER | QUIRK_RSTCHN | \
				 QUIRK_IGSTPOR | QUIRK_S3MRTG | QUIRK_MARKER )

/* DSP effects */
#define DSP_EFFECT_CUTOFF	0x02
#define DSP_EFFECT_RESONANCE	0x03
#define DSP_EFFECT_FILTER_A0	0xb0
#define DSP_EFFECT_FILTER_B0	0xb1
#define DSP_EFFECT_FILTER_B1	0xb2

/* Time factor */
#define DEFAULT_TIME_FACTOR	10.0
#define MED_TIME_FACTOR		2.64

#define MAX_SEQUENCES		16
#define MAX_SAMPLE_SIZE		0x10000000
#define MAX_SAMPLES		1024

#define IS_PLAYER_MODE_MOD()	(m->read_event_type == READ_EVENT_MOD)
#define IS_PLAYER_MODE_FT2()	(m->read_event_type == READ_EVENT_FT2)
#define IS_PLAYER_MODE_ST3()	(m->read_event_type == READ_EVENT_ST3)
#define IS_PLAYER_MODE_IT()	(m->read_event_type == READ_EVENT_IT)
#define IS_PLAYER_MODE_MED()	(m->read_event_type == READ_EVENT_MED)
#define IS_PERIOD_MODRNG()	(m->period_type == PERIOD_MODRNG)
#define IS_PERIOD_LINEAR()	(m->period_type == PERIOD_LINEAR)
#define IS_PERIOD_CSPD()	(m->period_type == PERIOD_CSPD)

#define IS_AMIGA_MOD()	(IS_PLAYER_MODE_MOD() && IS_PERIOD_MODRNG())

struct ord_data {
	int speed;
	int bpm;
	int gvl;
	int time;
	int start_row;
#ifndef LIBXMP_CORE_PLAYER
	int st26_speed;
#endif
};



/* Context */

struct smix_data {
	int chn;
	int ins;
	int smp;
	struct xmp_instrument *xxi;
	struct xmp_sample *xxs;
};

/* This will be added to the sample structure in the next API revision */
struct extra_sample_data {
	double c5spd;
};

struct module_data {
	struct xmp_module mod;

	char *dirname;			/* file dirname */
	char *basename;			/* file basename */
	char *filename;			/* Module file name */
	char *comment;			/* Comments, if any */
	uint8 md5[16];			/* MD5 message digest */
	int size;			/* File size */
	double rrate;			/* Replay rate */
	double time_factor;		/* Time conversion constant */
	int c4rate;			/* C4 replay rate */
	int volbase;			/* Volume base */
	int gvolbase;			/* Global volume base */
	int gvol;			/* Global volume */
	int *vol_table;			/* Volume translation table */
	int quirk;			/* player quirks */
#define READ_EVENT_MOD	0
#define READ_EVENT_FT2	1
#define READ_EVENT_ST3	2
#define READ_EVENT_IT	3
#define READ_EVENT_MED	4
	int read_event_type;
#define PERIOD_AMIGA	0
#define PERIOD_MODRNG	1
#define PERIOD_LINEAR	2
#define PERIOD_CSPD	3
	int period_type;
	int smpctl;			/* sample control flags */
	int defpan;			/* default pan setting */
	struct ord_data xxo_info[XMP_MAX_MOD_LENGTH];
	int num_sequences;
	struct xmp_sequence seq_data[MAX_SEQUENCES];
	char *instrument_path;
	void *extra;			/* format-specific extra fields */
	char **scan_cnt;		/* scan counters */
	struct extra_sample_data *xtra;
#ifndef LIBXMP_CORE_DISABLE_IT
	struct xmp_sample *xsmp;	/* sustain loop samples */
#endif
};


struct player_data {
	int ord;
	int pos;
	int row;
	int frame;
	int speed;
	int bpm;
	int mode;
	int player_flags;
	int flags;

	double current_time;
	double frame_time;

	int loop_count;
	int sequence;
	unsigned char sequence_control[XMP_MAX_MOD_LENGTH];

	int smix_vol;			/* SFX volume */
	int master_vol;			/* Music volume */
	int gvol;

	struct flow_control {
		int pbreak;
		int jump;
		int delay;
		int jumpline;
		int loop_chn;
	
		struct pattern_loop {
			int start;
			int count;
		} *loop;
	
		int num_rows;
		int end_point;
		int rowdelay;		/* For IT pattern row delay */
		int rowdelay_set;
	} flow;

	struct {
		int time;		/* replay time in ms */
		int ord;
		int row;
		int num;
	} scan[MAX_SEQUENCES];

	struct channel_data *xc_data;

	int channel_vol[XMP_MAX_CHANNELS];
	char channel_mute[XMP_MAX_CHANNELS];

	struct virt_control {
		int num_tracks;		/* Number of tracks */
		int virt_channels;	/* Number of virtual channels */
		int virt_used;		/* Number of voices currently in use */
		int maxvoc;		/* Number of sound card voices */
	
		struct virt_channel {
			int count;
			int map;
		} *virt_channel;
	
		struct mixer_voice *voice_array;
	} virt;

	struct xmp_event inject_event[XMP_MAX_CHANNELS];

	struct {		
		int consumed;
		int in_size;
		char *in_buffer;
	} buffer_data;

#ifndef LIBXMP_CORE_PLAYER
	int st26_speed;			/* For IceTracker speed effect */
#endif
	int filter;			/* Amiga led filter */
};

struct mixer_data {
	int freq;		/* sampling rate */
	int format;		/* sample format */
	int amplify;		/* amplification multiplier */
	int mix;		/* percentage of channel separation */
	int interp;		/* interpolation type */
	int dsp;		/* dsp effect flags */
	char* buffer;		/* output buffer */
	int32* buf32;		/* temporary buffer for 32 bit samples */
	int numvoc;		/* default softmixer voices number */
	int ticksize;
	int dtright;		/* anticlick control, right channel */
	int dtleft;		/* anticlick control, left channel */
	double pbase;		/* period base */
};

struct context_data {
	struct player_data p;
	struct mixer_data s;
	struct module_data m;
	struct smix_data smix;
	int state;
};


/* Prototypes */

char	*libxmp_adjust_string	(char *);
int	libxmp_exclude_match	(const char *);
int	libxmp_prepare_scan	(struct context_data *);
int	libxmp_scan_sequences	(struct context_data *);
int	libxmp_get_sequence	(struct context_data *, int);
int	libxmp_set_player_mode	(struct context_data *);

int8	read8s			(FILE *, int *err);
uint8	read8			(FILE *, int *err);
uint16	read16l			(FILE *, int *err);
uint16	read16b			(FILE *, int *err);
uint32	read24l			(FILE *, int *err);
uint32	read24b			(FILE *, int *err);
uint32	read32l			(FILE *, int *err);
uint32	read32b			(FILE *, int *err);
static inline void write8	(FILE *f, uint8 b) {
	fputc(b, f);
}
void	write16l		(FILE *, uint16);
void	write16b		(FILE *, uint16);
void	write32l		(FILE *, uint32);
void	write32b		(FILE *, uint32);
int	move_data		(FILE *, FILE *, int);

uint16	readmem16l		(const uint8 *);
uint16	readmem16b		(const uint8 *);
uint32	readmem24l		(const uint8 *);
uint32	readmem24b		(const uint8 *);
uint32	readmem32l		(const uint8 *);
uint32	readmem32b		(const uint8 *);

struct xmp_instrument *libxmp_get_instrument(struct context_data *, int);
struct xmp_sample *libxmp_get_sample(struct context_data *, int);

#endif /* LIBXMP_COMMON_H */
