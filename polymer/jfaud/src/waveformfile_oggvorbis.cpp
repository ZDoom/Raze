#define JFAUD_INTERNAL
#if USEVORBIS

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
# include "watcomhax/cerrno"
#else
# include <cstdlib>
# include <cerrno>
#endif

#include "log.h"
#include "pcmbuffer.hpp"
#include "waveformfile_oggvorbis.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#if !LINKVORBIS
#include "dynlib.hpp"
#define GETDLSYM(sym,sig) if (( libsyms.sym = (sig) lib->Get( (const char *)#sym )) == NULL) return getdlsymerr( #sym )
static bool getdlsymerr(const char *sym) { _JFAud_LogMsg("  Symbol %s not found. Vorbis disabled.\n", sym); return false; }
static DynamicLibrary *lib = NULL;
static int refcount = 0;

static struct {
	int (*ov_open_callbacks)(void *datasource, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks);
	int (*ov_clear)(OggVorbis_File *vf);
	vorbis_info *(*ov_info)(OggVorbis_File *vf,int link);
	long (*ov_seekable)(OggVorbis_File *vf);
	long (*ov_streams)(OggVorbis_File *vf);
	long (*ov_read)(OggVorbis_File *vf,char *buffer,int length, int bigendianp,int word,int sgned,int *bitstream);
	int (*ov_pcm_seek_lap)(OggVorbis_File *vf,ogg_int64_t pos);
	double (*ov_time_total)(OggVorbis_File *vf,int i);
	ogg_int64_t (*ov_pcm_total)(OggVorbis_File *vf,int i);
	vorbis_comment *(*ov_comment)(OggVorbis_File *vf,int i);
} libsyms;

static bool getallsyms()
{
	GETDLSYM(ov_open_callbacks,  int          (*)(void *,OggVorbis_File *,char *,long,ov_callbacks));
	GETDLSYM(ov_clear,           int          (*)(OggVorbis_File *));
	GETDLSYM(ov_info,            vorbis_info *(*)(OggVorbis_File *,int));
	GETDLSYM(ov_seekable,        long         (*)(OggVorbis_File *));
	GETDLSYM(ov_streams,         long         (*)(OggVorbis_File *));
	GETDLSYM(ov_read,            long         (*)(OggVorbis_File *,char *,int,int,int,int,int *));
	GETDLSYM(ov_pcm_seek_lap,    int          (*)(OggVorbis_File *,ogg_int64_t));
	GETDLSYM(ov_time_total,      double       (*)(OggVorbis_File *,int));
	GETDLSYM(ov_pcm_total,       ogg_int64_t  (*)(OggVorbis_File *,int));
	GETDLSYM(ov_comment,         vorbis_comment *(*)(OggVorbis_File *,int));
	return true;
}

#define ov_open_callbacks libsyms.ov_open_callbacks
#define ov_clear libsyms.ov_clear
#define ov_info libsyms.ov_info
#define ov_seekable libsyms.ov_seekable
#define ov_streams libsyms.ov_streams
#define ov_read libsyms.ov_read
#define ov_pcm_seek_lap libsyms.ov_pcm_seek_lap
#define ov_time_total libsyms.ov_time_total
#define ov_pcm_total libsyms.ov_pcm_total
#define ov_comment libsyms.ov_comment

#endif

//{{{ VorbisFile callbacks
static size_t vorbisread(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	int r;
	JFAudFile *file = (JFAudFile*)datasource;

	r = file->Read((int)(size * nmemb), ptr);

	if (r < 0) errno = 1, r = 0;
	else errno = 0;

	return (size_t)r;
}

static int vorbisseek(void *datasource, ogg_int64_t offset, int w)
{
	JFAudFile *file = (JFAudFile*)datasource;
	return file->Seek((int)offset, w==SEEK_CUR ? JFAudFile::Cur : (w==SEEK_SET ? JFAudFile::Set : JFAudFile::End));
}

static int vorbisclose(void *datasource)
{
	// deleting the fh in the destructor will close the file
	return 0;
}

static long vorbistell(void *datasource)
{
	JFAudFile *file = (JFAudFile*)datasource;
	return (long)file->Tell();
}
//}}}

//{{{ Private methods
int WaveformFile_OggVorbis::LoadSamples(PcmBuffer *buf, bool loop)
{
	int bytes, bitstr;
	unsigned int totalbytes = buf->GetMaxSamples() * buf->GetBlockSize(), readbytes = 0;
	char *sp = (char*)buf->GetData();
	bool breakloop = false;

	while (totalbytes > 0) {
		bytes = ov_read(&vfile, sp, totalbytes, B_BIG_ENDIAN, 2, 1, &bitstr);

		if (bytes == OV_HOLE) continue;	// ignore holes
		
		if (bytes < 0) return -1;	// read failure
		else if (bytes == 0) {
			if (loop) {
				if (breakloop) break;	// to prevent endless loops

				if (ov_pcm_seek_lap(&vfile, 0)) return -1;
				breakloop = true;
			} else {
				break;
			}
		} else {
			sp         += bytes;
			readbytes  += bytes;
			totalbytes -= bytes;
			breakloop = false;
		}
	}

	buf->SetNumSamples(readbytes / buf->GetBlockSize());

	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_OggVorbis::WaveformFile_OggVorbis(JFAudFile *tfh)
	: isvalid(false), WaveformFile(), fh(NULL)
{
	int r;
	vorbis_info *vi;
	ov_callbacks vorbiscallbacks = { vorbisread, vorbisseek, vorbisclose, vorbistell };

	if (!tfh) return;
#if !LINKVORBIS
	if (!lib) return;
#endif

	tfh->Rewind();
	r = ov_open_callbacks((void*)tfh, &vfile, NULL, 0, vorbiscallbacks);
	if (r < 0) return;

	vi = ov_info(&vfile, 0);
	if (!vi) {
		ov_clear(&vfile);
		return;
	}

	samplerate = vi->rate;
	channels   = vi->channels;
	
	fh = tfh;
	isvalid = true;
}

WaveformFile_OggVorbis::~WaveformFile_OggVorbis()
{
	if (isvalid) ov_clear(&vfile);
	if (fh) delete fh;
}

PcmBuffer *WaveformFile_OggVorbis::ReadSamples(unsigned int nsamps, bool loop)
{
	PcmBuffer *buf;

	if (!isvalid) return NULL;

	buf = new PcmBuffer();
	if (!buf) return NULL;

	if (nsamps == 0) {
		nsamps = (unsigned int)ov_pcm_total(&vfile,-1);
		loop = false;
	}

	if (!buf->Allocate(nsamps, samplerate, channels, 2) ||
	    LoadSamples(buf, loop) < 0 ||
	    buf->GetNumSamples() == 0) {
		delete buf;
		return NULL;
	}

	return buf;
}

float WaveformFile_OggVorbis::GetPlayTime(void) const
{
	double r;

	if (!isvalid) return 0.0;

	r = ov_time_total(const_cast<OggVorbis_File*>(&vfile),-1);
	if (r == OV_EINVAL) return 0.0;

	return (float)r;
}

unsigned int WaveformFile_OggVorbis::GetPCMLength(void) const
{
	ogg_int64_t r;

	if (!isvalid) return 0;
	
	r = ov_pcm_total(const_cast<OggVorbis_File*>(&vfile),-1);
	if (r == OV_EINVAL) return 0;

	return (unsigned int)r;
}

SoundFile::InitState WaveformFile_OggVorbis::Init()
{
#if !LINKVORBIS
	if (lib) { refcount++; return SoundFile::InitOK; }
	
	_JFAud_LogMsg("Loading " VORBISDL "\n");
	lib = new DynamicLibrary(VORBISDL);
	if (!lib) return SoundFile::InitDisabled;
	if (!lib->IsOpen()) {
		delete lib;
		lib = NULL;
		return SoundFile::InitDisabled;
	}

	if (getallsyms()) refcount = 1;
	else {
		delete lib;
		lib = NULL;
		return SoundFile::InitDisabled;
	}
#endif
	return SoundFile::InitOK;
}

bool WaveformFile_OggVorbis::Uninit()
{
#if !LINKVORBIS
	if (refcount > 1) { refcount--; return true; }
	if (refcount == 0 || !lib) return false;
	refcount = 0;
	delete lib;
	lib = NULL;
#endif
	return true;
}
//}}}

#endif

// vim:fdm=marker:

