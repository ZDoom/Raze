#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
#else
# include <cstdio>
#endif

#include "log.h"
#include "soundfile.hpp"
#include "waveformfile.hpp"

#include "waveformfile_riffwave.hpp"
#include "waveformfile_aiff.hpp"
#include "waveformfile_voc.hpp"
#include "waveformfile_au.hpp"
#include "waveformfile_oggvorbis.hpp"
#include "waveformfile_flac.hpp"
#include "waveformfile_mpeg.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

static int init = 0;

bool InitialiseWaveformReaders(void)
{
	int i;
	SoundFile::InitState r;
	const char *name;

	if (init > 0) return true;

	_JFAud_LogMsg("Supported waveform formats:\n");
	for (i=WaveformFile::FORMAT_FIRST; i<=WaveformFile::FORMAT_LAST; i++) {
		switch (i) {
			case WaveformFile::FORMAT_RAW:     /*r = WaveformFile_Raw::Init();*/ continue;
			case WaveformFile::FORMAT_RIFFWAV:
				r    = WaveformFile_RiffWave::Init();
				name = WaveformFile_RiffWave::GetClassFormatName();
				break;
			case WaveformFile::FORMAT_AIFF:
				r    = WaveformFile_Aiff::Init();
				name = WaveformFile_Aiff::GetClassFormatName();
				break;
			case WaveformFile::FORMAT_VOC:
				r    = WaveformFile_Voc::Init();
				name = WaveformFile_Voc::GetClassFormatName();
				break;
			case WaveformFile::FORMAT_AU:
				r    = WaveformFile_Au::Init();
				name = WaveformFile_Au::GetClassFormatName();
				break;
#if USEVORBIS
			case WaveformFile::FORMAT_OGGVORBIS:
				r    = WaveformFile_OggVorbis::Init();
				name = WaveformFile_OggVorbis::GetClassFormatName();
				break;
#endif
#if USEFLAC
			case WaveformFile::FORMAT_FLAC:
				r    = WaveformFile_Flac::Init();
				name = WaveformFile_Flac::GetClassFormatName();
				break;
#endif
#if USEMPADEC
			case WaveformFile::FORMAT_MPEG:
				r    = WaveformFile_Mpeg::Init();
				name = WaveformFile_Mpeg::GetClassFormatName();
				break;
#endif
			default: continue;
		}

		switch (r) {
			case SoundFile::InitFailed: return false;
			case SoundFile::InitOK: _JFAud_LogMsg("   %s\n", name); break;
			case SoundFile::InitDisabled: _JFAud_LogMsg("   %s (disabled)\n", name); break;
		}
	}
	init++;
	return true;
}

void UninitialiseWaveformReaders(void)
{
	int i;

	if (init <= 0) return;

	for (i=WaveformFile::FORMAT_FIRST; i<=WaveformFile::FORMAT_LAST; i++) {
		switch (i) {
			case WaveformFile::FORMAT_RAW:     /*WaveformFile_Raw::Uninit();*/ continue;
			case WaveformFile::FORMAT_RIFFWAV: WaveformFile_RiffWave::Uninit(); break;
			case WaveformFile::FORMAT_AIFF:    WaveformFile_Aiff::Uninit(); break;
			case WaveformFile::FORMAT_VOC:     WaveformFile_Voc::Uninit(); break;
			case WaveformFile::FORMAT_AU:      WaveformFile_Au::Uninit(); break;
#if USEVORBIS
			case WaveformFile::FORMAT_OGGVORBIS: WaveformFile_OggVorbis::Uninit(); break;
#endif
#if USEFLAC
			case WaveformFile::FORMAT_FLAC:    WaveformFile_Flac::Uninit(); break;
#endif
#if USEMPADEC
			case WaveformFile::FORMAT_MPEG:    WaveformFile_Mpeg::Uninit(); break;
#endif
			default: continue;
		}
	}
	init--;
}

WaveformFile *IdentifyWaveformFile(JFAudFile *fh)
{
	int i;
	WaveformFile *sfile;
	for (i=WaveformFile::FORMAT_FIRST; i<=WaveformFile::FORMAT_LAST; i++) {
		switch (i) {
			case WaveformFile::FORMAT_RAW: continue;
			case WaveformFile::FORMAT_RIFFWAV:   sfile = new WaveformFile_RiffWave(fh); break;
			case WaveformFile::FORMAT_AIFF:      sfile = new WaveformFile_Aiff(fh); break;
			case WaveformFile::FORMAT_VOC:       sfile = new WaveformFile_Voc(fh); break;
			case WaveformFile::FORMAT_AU:        sfile = new WaveformFile_Au(fh); break;
#if USEVORBIS
			case WaveformFile::FORMAT_OGGVORBIS: sfile = new WaveformFile_OggVorbis(fh); break;
#endif
#if USEFLAC
			case WaveformFile::FORMAT_FLAC:      sfile = new WaveformFile_Flac(fh); break;
#endif
#if USEMPADEC
			case WaveformFile::FORMAT_MPEG:      sfile = new WaveformFile_Mpeg(fh); break;
#endif
			default: continue;
		}

		if (!sfile) return NULL;
		if (!sfile->IsValid()) delete sfile;
		else if (!sfile->IsUsable()) {
			delete sfile;
			return NULL;
		} else return sfile;
	}
	return NULL;
}

