#ifdef __APPLE__
# include <jfaud/jfaud.hpp>
#else
# include "jfaud.hpp"
#endif

#include <ctype.h>

#include "compat.h"
#include "baselayer.h"
#include "cache1d.h"

extern "C" {
void initsb(char,char,long,char,char,char,char);
void uninitsb(void);
void setears(long,long,long,long);
void wsayfollow(char *,long,long,long *,long *,char);
void wsay(char *,long,long,long);
void loadwaves(void);
void loadsong(char *);
void musicon(void);
void musicoff(void);
void refreshaudio(void);
}
	
#define NUMCHANNELS 16
#define UNITSPERMTR 640.0
#define WAVESFILE "WAVES.KWV"

static char musistat = 0;
static char songname[BMAX_PATH] = "";
static JFAud *jfaud = NULL;

static int kwvnumwaves = 0;
static struct _kwvitem {
	char instname[16];
	long wavleng;
	long repstart;
	long repleng;
	long finetune;
	long datastart;
} *kwvitems = NULL;

static struct {
	long *posx, *posy;
	JFAudMixerChannel *handle;
} sfxchans[NUMCHANNELS];

class KenFile : public JFAudFile {
private:
	int fh;
public:
	KenFile(const char *filename, const char *subfilename)
		: JFAudFile(filename, subfilename)
	{
		fh = kopen4load(const_cast<char*>(filename), 0);
	}

	virtual ~KenFile()
	{
		if (fh >= 0) kclose(fh);
	}

	virtual bool IsOpen(void) const { return fh >= 0; }

	virtual long Read(long nbytes, void *buf)
	{
		if (fh < 0) return -1;
		return kread(fh, buf, nbytes);
	}
	virtual long Seek(long pos, SeekFrom where)
	{
		int when;
		if (fh < 0) return -1;
		switch (where) {
			case JFAudFile::Set: when = SEEK_SET; break;
			case JFAudFile::Cur: when = SEEK_CUR; break;
			case JFAudFile::End: when = SEEK_END; break;
			default: return -1;
		}
		return klseek(fh, pos, when);
	}
	virtual long Tell(void) const
	{
		if (fh < 0) return -1;
		return klseek(fh, 0, SEEK_CUR);
	}
	virtual long Length(void) const
	{
		if (fh < 0) return -1;
		return kfilelength(fh);
	}
};

class KWVFile : public JFAudFile {
private:
	int fh;
	long datastart, datalen, datapos;
public:
	KWVFile(const char *filename, const char *subfilename = NULL)
		: JFAudFile(filename, NULL),
		  datastart(-1), datalen(-1), datapos(-1)
	{
		long i,j;
		bool found = false;

		if (!subfilename) return;

		for (i=0;i<kwvnumwaves;i++) {
			for (j=0;j<16 && subfilename[j];j++) {
				if (tolower(subfilename[j]) != tolower(kwvitems[i].instname[j])) {
					found = false;
					break;
				} else found = true;
			}
			if (found) {
				fh = kopen4load(const_cast<char *>(filename), 0);
				if (fh < 0) return;
				
				datastart = kwvitems[i].datastart;
				datalen   = kwvitems[i].wavleng;
				datapos   = 0;
				klseek(fh, datastart, SEEK_SET);
				return;
			}
		}
	}
	
	virtual ~KWVFile()
	{
		if (fh >= 0) kclose(fh);
	}
	virtual bool IsOpen(void) const
	{
		return datalen > 0 && fh >= 0;
	}

	virtual long Read(long nbytes, void *buf)
	{
		if (!IsOpen()) return -1;
		if (datalen - datapos < nbytes) nbytes = datalen - datapos;
		if (nbytes <= 0) return 0;
		nbytes = kread(fh, buf, nbytes);
		if (nbytes > 0) datapos += nbytes;
		return nbytes;
	}
	virtual long Seek(long pos, SeekFrom where)
	{
		long newpos;
		if (!IsOpen()) return -1;
		switch (where) {
			case JFAudFile::Set: newpos = pos; break;
			case JFAudFile::Cur: newpos = datapos + pos; break;
			case JFAudFile::End: newpos = datalen + pos; break;
			default: return -1;
		}
		if (newpos < 0) newpos = 0;
		else if (newpos > datalen) newpos = datalen;
		return klseek(fh, datastart + newpos, SEEK_SET);
	}
	virtual long Tell(void) const
	{
		if (!IsOpen()) return -1;
		return datapos;
	}
	virtual long Length(void) const
	{
		if (!IsOpen()) return -1;
		return datalen;
	}
};

static JFAudFile *openfile(const char *fn, const char *subfn)
{
	char *ext;
	bool loadkwv = false;
	
	ext = Bstrrchr(fn,'.');
	if (!ext || Bstrcasecmp(ext, ".kwv"))
		return static_cast<JFAudFile*>(new KenFile(fn,subfn));
	if (!subfn) return NULL;	// KWV files need a sub name

	return static_cast<JFAudFile*>(new KWVFile(fn, subfn));
}

static void logfunc(const char *s) { initprintf("%s", s); }

void loadwaves(void)
{
	long fh, i, datastart;

	fh = kopen4load(WAVESFILE, 0);
	if (fh < 0) return;
	
	if (kread(fh, &i,           4) != 4 || i != 0) return;
	if (kread(fh, &kwvnumwaves, 4) != 4) return; kwvnumwaves = B_LITTLE32(kwvnumwaves);
	
	kwvitems = new struct _kwvitem [kwvnumwaves];
	if (!kwvitems) {
		kclose(fh);
		return;
	}
	
	datastart = (4+4) + kwvnumwaves * (16+4*4);
	for (i=0;i<kwvnumwaves;i++) {
		if (kread(fh,  kwvitems[i].instname, 16) != 16) return;
		if (kread(fh, &kwvitems[i].wavleng,  4)  != 4)  return; kwvitems[i].wavleng  = B_LITTLE32(kwvitems[i].wavleng);
		if (kread(fh, &kwvitems[i].repstart, 4)  != 4)  return; kwvitems[i].repstart = B_LITTLE32(kwvitems[i].repstart);
		if (kread(fh, &kwvitems[i].repleng,  4)  != 4)  return; kwvitems[i].repleng  = B_LITTLE32(kwvitems[i].repleng);
		if (kread(fh, &kwvitems[i].finetune, 4)  != 4)  return; kwvitems[i].finetune = B_LITTLE32(kwvitems[i].finetune);
		kwvitems[i].datastart = datastart;
		datastart += kwvitems[i].wavleng;
	}
	
	kclose(fh);
}

void initsb(char dadigistat, char damusistat, long dasamplerate, char danumspeakers, 
		char dabytespersample, char daintspersec, char daquality)
{
	int i;

	if (jfaud) return;

	JFAud_SetLogFunc(logfunc);

	jfaud = new JFAud();
	if (!jfaud) return;

	jfaud->SetUserOpenFunc(openfile);

	musistat = 0;
	if (!jfaud->InitWave(NULL, NUMCHANNELS, dasamplerate)) {
		delete jfaud;
		jfaud = NULL;
		return;
	}

	musistat = damusistat;

	for (i=NUMCHANNELS-1;i>=0;i--) sfxchans[i].handle = NULL;
	
	loadwaves();
}

void uninitsb(void)
{
	if (!jfaud) return;

	delete jfaud;
	jfaud = NULL;
	
	if (kwvitems) delete [] kwvitems;
}

void setears(long daposx, long daposy, long daxvect, long dayvect)
{
	JFAudMixer *mixer;

	if (!jfaud) return;
	mixer = jfaud->GetWave();
	if (!mixer) return;

	mixer->SetListenerPosition((float)daposx/UNITSPERMTR, 0.0, (float)-daposy/UNITSPERMTR);
	mixer->SetListenerOrientation((float)daxvect/UNITSPERMTR, 0.0, (float)-dayvect/UNITSPERMTR,
			0.0, 1.0, 0.0);
}

static int storehandle(JFAudMixerChannel *handle, long *daxplc, long *dayplc)
{
	int i,empty = -1;

	for (i=NUMCHANNELS-1;i>=0;i--) {
		if (!sfxchans[i].handle && empty<0) empty = i;
		if (sfxchans[i].handle == handle) {
			empty = i;
			break;
		}
	}

	if (empty < 0) return -1;

	sfxchans[empty].handle = handle;
	sfxchans[empty].posx = daxplc;
	sfxchans[empty].posy = dayplc;
	
	return empty;
}

static void stopcb(int r)
{
	jfaud->FreeSound(sfxchans[r].handle);
	sfxchans[r].handle = NULL;
}

void wsayfollow(char *dafilename, long dafreq, long davol, long *daxplc, long *dayplc, char followstat)
{
	JFAudMixerChannel *handl;
	int r;

	if (!jfaud) return;

	handl = jfaud->PlayRawSound(WAVESFILE, dafilename, 1, 11025, 1, 1, false);
	if (!handl) return;

	if (followstat) r = storehandle(handl, daxplc, dayplc);
	else r = storehandle(handl, NULL, NULL);
	
	if (r >= 0) handl->SetStopCallback(stopcb, r);

	handl->SetPitch((float)dafreq / 4096.0);
	handl->SetGain((float)davol / 256.0);
	handl->SetPosition((float)(*daxplc) / UNITSPERMTR, 0.0, (float)(-*dayplc) / UNITSPERMTR);
	handl->SetFollowListener(false);
	handl->SetRolloff(1.0);

	handl->Play();
}

void wsay(char *dafilename, long dafreq, long volume1, long volume2)
{
	JFAudMixerChannel *handl;
	int r;

	if (!jfaud) return;
	
	handl = jfaud->PlayRawSound(WAVESFILE, dafilename, 1, 11025, 1, 1, false);
	if (!handl) return;

	r = storehandle(handl, NULL, NULL);
	
	if (r >= 0) handl->SetStopCallback(stopcb, r);
	
	handl->SetPitch((float)dafreq / 4096.0);
	handl->SetGain((float)volume1 / 256.0);
	handl->SetPosition(0.0, 0.0, 0.0);
	handl->SetFollowListener(true);
	handl->SetRolloff(0.0);

	handl->Play();
}

void loadsong(char *filename)
{
	if (!jfaud) return;
	strcpy(songname,filename);
}

void musicon(void)
{
	if (!jfaud || !musistat || !songname[0]) return;

	jfaud->PlayMusic(songname);
}

void musicoff(void)
{
	if (!jfaud || !musistat) return;

	jfaud->StopMusic();
}

void refreshaudio(void)
{
	int i;

	if (!jfaud) return;
	for (i=NUMCHANNELS-1;i>=0;i--) {
		if (!sfxchans[i].handle || !jfaud->IsValidSound(sfxchans[i].handle)) continue;
		if (!sfxchans[i].posx || !sfxchans[i].posy) continue;

		sfxchans[i].handle->SetPosition(
				(float)(*sfxchans[i].posx)/UNITSPERMTR,
				0.0,
				(float)(-*sfxchans[i].posy)/UNITSPERMTR);
	}
	jfaud->Update();
}
