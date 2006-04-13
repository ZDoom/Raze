// FMOD Sound Interface
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#include "fmod.h"
#include "fmod_errors.h"

#ifndef F_CALLBACKAPI
#define FMOD_PRE370
#define F_CALLBACKAPI _cdecl
#endif

// some Linux people may need this if GCC can't find FSOUND_Stream_Open
// #define FMOD_PRE370

#include "osd.h"
#include "compat.h"
#include "cache1d.h"

#define NUMCHANNELS 16
#define MAXWAVES 256


static int fmod_inited = 0;
static int musicstat = 0;

// general sample playback
static int numwaves;
static FSOUND_SAMPLE * samples[MAXWAVES];
static char instname[MAXWAVES][16];
static int channels[NUMCHANNELS];

void loadwaves(void);




static unsigned int F_CALLBACKAPI f_open(const char *name)
{
	return kopen4load((char *)name, 0) + 1;
}

static void F_CALLBACKAPI f_close(unsigned int handle)
{
	kclose(handle - 1);
}

static int F_CALLBACKAPI f_read(void *buffer, int size, unsigned int handle)
{
	return kread(handle - 1, buffer, size);
}

static int F_CALLBACKAPI f_seek(unsigned int handle, int pos, signed char mode)
{
	return klseek(handle - 1, pos, mode);
}

static int F_CALLBACKAPI f_tell(unsigned int handle)
{
	return ktell(handle - 1);
}


void initsb(char dadigistat, char damusistat, long dasamplerate, char danumspeakers, char dabytespersample, char daintspersec, char daquality)
{
	char *s;
	int i,j;
	
	if (fmod_inited) return;
	fmod_inited = 0;

	if (dasamplerate < 6000) dasamplerate = 6000;
	else if (dasamplerate > 48000) dasamplerate = 48000;

	musicstat = damusistat;
	
	printOSD("Initialising FMOD...\n");
	printOSD("  Linked version: %.02f\n", FMOD_VERSION);
	printOSD("  DLL version: %.02f\n", FSOUND_GetVersion());

	if (FSOUND_GetVersion() < FMOD_VERSION) {
		printOSD("  ... Failure: FMOD DLL too old! Sound disabled.\n");
		return;
	}
	
	printOSD("  Samplerate: %d hz\n", dasamplerate);

	//FSOUND_SetOutput(FSOUND_OUTPUT_ASIO);

	if (FSOUND_Init(dasamplerate, NUMCHANNELS, 0)) {
		printOSD("  ... Success\n");
		fmod_inited = 1;
	} else {
		printOSD("  ... Failure: %s\n", FMOD_ErrorString(FSOUND_GetError()));
	}

	switch (FSOUND_GetOutput()) {
		case FSOUND_OUTPUT_NOSOUND: s = "No Sound"; break;
		case FSOUND_OUTPUT_WINMM: s = "WINMM"; break;
		case FSOUND_OUTPUT_DSOUND: s = "DirectSound"; break;
		case FSOUND_OUTPUT_OSS: s = "OSS"; break;
		case FSOUND_OUTPUT_ESD: s = "ESound"; break;
		case FSOUND_OUTPUT_ALSA: s = "ALSA"; break;
		case FSOUND_OUTPUT_ASIO: s = "ASIO"; break;
		default: s = "Other"; break;
	}
	printOSD("Using FMOD \"%s\" output driver\n", s);

	FSOUND_File_SetCallbacks(
			(FSOUND_OPENCALLBACK)f_open,
			(FSOUND_CLOSECALLBACK)f_close,
			(FSOUND_READCALLBACK)f_read,
			(FSOUND_SEEKCALLBACK)f_seek,
			(FSOUND_TELLCALLBACK)f_tell);
	//FSOUND_SetMemorySystem(fmod_cache, fmod_cachelen, NULL, NULL, NULL);

	loadwaves();

	for (i=0; i<NUMCHANNELS; i++) channels[i] = -1;
}

void uninitsb(void)
{
	if (fmod_inited) {		
		FSOUND_Close();
		fmod_inited = 0;
	}
}


void setears(long daposx, long daposy, long daxvect, long dayvect)
{
}


void wsayfollow(char *dafilename, long dafreq, long davol, long *daxplc, long *dayplc, char followstat)
{
/*
	dafilename:
		filename

	dafreq:
		4096 = Middle C
		8192 = +1 8ve

	davol:
		0-256 = volume

	daxplc:
	dayplc:
		followstat: 0 = sound emination origin (fixed)
		followstat: 1 = pointer to sprite coordinates (dynamic)

	followstat:
		0 = position is fixed in space
		1 = position follows an object
*/
	unsigned char ch1, ch2;
	long i, wavnum, bad, oldest=0, free=-1, ox, oy, x, y, vo;
	unsigned int oldestpos=0;
	int chan;

	if (fmod_inited == 0) return;

	for (wavnum=numwaves-1;wavnum>=0;wavnum--) {
		bad = 0;

		i = 0;
		while ((dafilename[i] > 0) && (i < 16))
		{
			ch1 = dafilename[i]; if ((ch1 >= 97) && (ch1 <= 123)) ch1 -= 32;
			ch2 = instname[wavnum][i]; if ((ch2 >= 97) && (ch2 <= 123)) ch2 -= 32;
			if (ch1 != ch2) {bad = 1; break;}
			i++;
		}
		if (bad != 0) continue;

		for (i=0; i<NUMCHANNELS; i++) {
			if (!FSOUND_IsPlaying(channels[i])) {
				free = i;
				break;
			}
			if (i==0) continue;
			
			if (FSOUND_GetCurrentPosition(channels[i]) > oldestpos) {
				oldest = i;
				oldestpos = FSOUND_GetCurrentPosition(channels[i]);
			}
		}

		if (free < 0) {
			FSOUND_StopSound(channels[oldest]);
			free = oldest;
		}

		chan = FSOUND_PlaySoundEx(FSOUND_FREE, samples[wavnum], NULL, 1);
		if (chan == -1) return;
		FSOUND_SetFrequency(chan, dafreq*11025/4096);
		FSOUND_SetVolume(chan, davol);
		
		FSOUND_SetPaused(chan, 0);
		
		channels[free] = chan;
		return;
	}
}


void wsay(char *dafilename, long dafreq, long volume1, long volume2)
{
	unsigned char ch1, ch2;
	long i, j, bad, free=-1, oldest=0;
	unsigned int oldestpos=0;
	int chan;

	if (fmod_inited == 0) return;

	i = numwaves-1;
	do
	{
		bad = 0;

		j = 0;
		while ((dafilename[j] > 0) && (j < 16))
		{
			ch1 = dafilename[j]; if ((ch1 >= 97) && (ch1 <= 123)) ch1 -= 32;
			ch2 = instname[i][j]; if ((ch2 >= 97) && (ch2 <= 123)) ch2 -= 32;
			if (ch1 != ch2) {bad = 1; break;}
			j++;
		}
		if (bad == 0)
		{
			for (j=0; j<NUMCHANNELS; j++) {
				if (!FSOUND_IsPlaying(channels[j])) {
					free = j;
					break;
				}
				if (j==0) continue;
			
				if (FSOUND_GetCurrentPosition(channels[j]) > oldestpos) {
					oldest = j;
					oldestpos = FSOUND_GetCurrentPosition(channels[j]);
				}
			}

			if (free < 0) {
				FSOUND_StopSound(channels[oldest]);
				free = oldest;
			}

			chan = FSOUND_PlaySoundEx(FSOUND_FREE, samples[i], NULL, 1);
			if (chan == -1) return;
			FSOUND_SetFrequency(chan, dafreq*11025/4096);
			FSOUND_SetVolume(chan, (volume1*volume2)>>1);
			// set pan
			FSOUND_SetPaused(chan, 0);

			channels[free] = chan;
			
			return;
		}

		i--;
	} while (i >= 0);
}


void loadwaves(void)
{
	long fil, dawaversionum, i, tmp;
	long wavleng[MAXWAVES], repstart[MAXWAVES], repleng[MAXWAVES], finetune[MAXWAVES];
	char *p;

	fil = kopen4load("WAVES.KWV", 0);

	if (fil != -1) {
		kread(fil, &dawaversionum, 4); dawaversionum = B_LITTLE32(dawaversionum);
		if (dawaversionum != 0) { kclose(fil); return; }

		kread(fil, &numwaves, 4); numwaves = B_LITTLE32(numwaves);
		for (i=0; i<numwaves; i++) {
			kread(fil, &instname[i][0], 16);
			kread(fil, &wavleng[i], 4);  wavleng[i]  = B_LITTLE32(wavleng[i]);
			kread(fil, &repstart[i], 4); repstart[i] = B_LITTLE32(repstart[i]);
			kread(fil, &repleng[i], 4);  repleng[i]  = B_LITTLE32(repleng[i]);
			kread(fil, &finetune[i], 4); finetune[i] = B_LITTLE32(finetune[i]);
		}
	} else {
		dawaversionum = 0;
		numwaves = 0;
	}

	for (i=numwaves; i<MAXWAVES; i++) {
		memset(&instname[i][0], 0, 16);
		wavleng[i] = 0;
		repstart[i] = 0;
		repleng[i] = 0;
		finetune[i] = 0;
		samples[i] = NULL;
	}

	if (fil == -1) return;

	for (i=0; i<numwaves; i++) {
		if (repleng[i]) tmp = FSOUND_LOOP_NORMAL;
		else tmp = FSOUND_LOOP_OFF;
		samples[i] = FSOUND_Sample_Alloc(FSOUND_FREE, wavleng[i], tmp, 11025, 255, 128, 1);
		if (!samples[i]) continue;

		p = (char*)Bmalloc(wavleng[i]);
		kread(fil,p,wavleng[i]);
		FSOUND_Sample_Upload(samples[i], p, FSOUND_8BITS | FSOUND_MONO | FSOUND_UNSIGNED);
		Bfree(p);

		if (repleng[i]) FSOUND_Sample_SetLoopPoints(samples[i], repstart[i], repstart[i]+repleng[i]);
	}

	kclose(fil);

	printOSD("Loaded %d waves\n", numwaves);
}


#if 1
static FSOUND_STREAM *musicstream = NULL;
static int musicplaying = 0;

void loadsong(char *filename)
{
	if (!musicstat) return;
	if (musicstream) return;
#ifdef FMOD_PRE370
	musicstream = FSOUND_Stream_OpenFile(filename, FSOUND_LOOP_NORMAL, 0);
#else
	musicstream = FSOUND_Stream_Open(filename, FSOUND_LOOP_NORMAL, 0, 0);
#endif
}

void musicon(void)
{
	if (!musicstat) return;
	if (!musicstream || musicplaying) return;
	FSOUND_Stream_Play(FSOUND_FREE, musicstream);
	musicplaying = 1;
}

void musicoff(void)
{
	if (!musicstat) return;
	if (!musicstream || !musicplaying) return;
	FSOUND_Stream_Stop(musicstream);
	musicplaying = 0;
}
#endif

void refreshaudio(void)
{
}
