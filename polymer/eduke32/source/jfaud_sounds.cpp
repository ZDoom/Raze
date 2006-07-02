/*
 * Audio support for JFDuke3D using JFAud
 * by Jonathon Fowler (jonof@edgenetwork.org)
 *
 * Duke Nukem 3D is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Original Source: 1996 - Todd Replogle
 * Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
 */

#include "types.h"
#include "duke3d.h"
extern "C" {
#ifdef RENDERTYPEWIN
# include "winlayer.h"
#endif
#include "osd.h"
long numenvsnds;
}

typedef uint64 uint64_t;
#ifdef __APPLE__
# include <jfaud/jfaud.hpp>
#else
# include "jfaud.hpp"
#endif

#define SOUNDM_LOOP   1
#define SOUNDM_MSFX   2
#define SOUNDM_DUKE   4
#define SOUNDM_PARENT 8
#define SOUNDM_GLOBAL 16
#define SOUNDM_NICE   64	// Added for JFDuke3D so JFAud doesn't use nearest filtering for the sound
#define SOUNDM_PLAYER 128

#define UNITSPERMETRE 512.0

#include <cmath>

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

static JFAudFile *openfile(const char *fn, const char *subfn)
{
	return static_cast<JFAudFile*>(new KenFile(fn,subfn));
}

static void logfunc(const char *s) { initprintf("jfaud: %s", s); }

#define PITCHRANGE 2		// octave range in each direction
#define PITCHSTEPS 24		// pitch points per octave
static float pitchtable[PITCHRANGE*PITCHSTEPS*2+1];
static void buildpitchtable(void)
{
	int i,j;
	
	for (i=-PITCHRANGE*PITCHSTEPS; i<=PITCHRANGE*PITCHSTEPS; i++) {
		pitchtable[i+PITCHRANGE*PITCHSTEPS] = pow(1.0005777895, (1200.0/PITCHSTEPS)*(float)i);
	}
}
static float translatepitch(int p)
{
	float t;
	int x;
	x = (p * PITCHSTEPS / 1200) + PITCHRANGE*PITCHSTEPS;
	if (x < 0) x = 0;
	else if (x > (int)(sizeof(pitchtable)/sizeof(float))) x = sizeof(pitchtable)/sizeof(float);
	t = pitchtable[x];
	/*if (t > 2.0) {
		initprintf("translatepitch(%d) > 2.0\n", p);
		t = 2.0;
	}*/
	return t;
}

typedef struct {
	JFAudMixerChannel *chan;
	int owner;	// sprite number
	int soundnum;	// sound number
} SoundChannel;

static SoundChannel *chans = NULL;
static JFAud *jfaud = NULL;

static bool havemidi = false, havewave = false;


static void stopcallback(int r)
{
	jfaud->FreeSound(chans[r].chan);
	chans[r].chan = NULL;
	chans[r].owner = -1;
}

void testcallback(unsigned long num)
{
}

static int keephandle(JFAudMixerChannel *handle, int soundnum, int owner)
{
	int i, freeh=-1;
	for (i=NumVoices-1;i>=0;i--) {
		if ((!chans[i].chan || !jfaud->IsValidSound(chans[i].chan)) && freeh<0) freeh=i;
		else if (chans[i].chan == handle) { freeh=i; break; }
	}
	if (freeh<0) {
		initprintf("Warning: keephandle() exhausted handle space!\n");
		return -1;
	}

	chans[freeh].chan = handle;
	chans[freeh].soundnum = soundnum;
	chans[freeh].owner = owner;
	
	return freeh;
}

void SoundStartup(void)
{
	int i;

	if (FXDevice < 0) return;

	if (jfaud) return;
	buildpitchtable();

	JFAud_SetLogFunc(logfunc);

	jfaud = new JFAud();
	if (!jfaud) return;

	jfaud->SetUserOpenFunc(openfile);
#ifdef _WIN32
	jfaud->SetWindowHandle((void*)win_gethwnd());
#endif

	havewave = havemidi = false;
	if (!jfaud->InitWave("software", NumVoices, MixRate)) {
		delete jfaud;
		jfaud = NULL;
		return;
	}

	{
		// the engine will take 60% of the system memory size for cache1d if there
		// is less than the 16MB asked for in loadpics(), so we'll
		// take 30% of what's left for the sound cache if that happened, or
		// 50% of the system memory sans the 16MB maximum otherwise
		unsigned k;
		if (Bgetsysmemsize() <= MAXCACHE1DSIZE)
			k = Bgetsysmemsize()/100*30;
		else
			k = Bgetsysmemsize()/100*50 - MAXCACHE1DSIZE;
		jfaud->SetCacheSize(k,k/2);
		jfaud->SetCacheItemAge(24*120);	// 24 movements per second, 120 seconds max lifetime
	}
	
	chans = new SoundChannel[NumVoices];
	if (!chans) {
		delete jfaud;
		jfaud = NULL;
		return;
	}

	havewave = true;

	for (i=NumVoices-1; i>=0; i--) {
		chans[i].owner = -1;
	}
	
	if (jfaud->InitMIDI(NULL)) havemidi = true;
}

void SoundShutdown(void)
{
	if (jfaud) delete jfaud;
	if (chans) delete [] chans;
	jfaud = NULL;
	chans = NULL;
	havewave = havemidi = false;
}

void MusicStartup(void)
{
}

void MusicShutdown(void)
{
}

void AudioUpdate(void)
{
	int i;

	if (!jfaud) return;
	if (havewave)
	for (i=NumVoices-1; i>=0; i--) {
		if (chans[i].chan && !jfaud->IsValidSound(chans[i].chan))
			chans[i].chan = NULL;
	}
	jfaud->Update(false);	// don't age the cache here
}


static char menunum = 0;
void intomenusounds(void)
{
	short i;
	short menusnds[] = {
		LASERTRIP_EXPLODE,	DUKE_GRUNT,			DUKE_LAND_HURT,
		CHAINGUN_FIRE,		SQUISHED,			KICK_HIT,
		PISTOL_RICOCHET,	PISTOL_BODYHIT,		PISTOL_FIRE,
		SHOTGUN_FIRE,		BOS1_WALK,			RPG_EXPLODE,
		PIPEBOMB_BOUNCE,	PIPEBOMB_EXPLODE,	NITEVISION_ONOFF,
		RPG_SHOOT,			SELECT_WEAPON
	};
	sound(menusnds[menunum++]);
	menunum %= sizeof(menusnds)/sizeof(menusnds[0]);
}

void playmusic(char *fn)
{
	char dafn[BMAX_PATH], *dotpos;
	int i;
	const char *extns[] = { ".ogg",".mp3",".mid", NULL };

	if (!MusicToggle) return;
	if (!jfaud) return;

	dotpos = Bstrrchr(fn,'.');
	if (dotpos && Bstrcasecmp(dotpos,".mid")) {
		// has extension but isn't midi
		jfaud->PlayMusic(fn, NULL);
	} else {
		Bstrcpy(dafn,fn);
		dotpos = Bstrrchr(dafn,'.');
		if (!dotpos) dotpos = dafn+strlen(dafn);

		for (i=0; extns[i]; i++) {
			Bstrcpy(dotpos, extns[i]);
			if (jfaud->PlayMusic(dafn, NULL)) return;
		}
	}
}

char loadsound(unsigned short num) { return 1; }

int isspritemakingsound(short i, int num)	// if num<0, check if making any sound at all
{
	int j,n=0;
	
	if (!jfaud || !havewave) return 0;
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) continue;
		if (chans[j].owner == i)
			if (num < 0 || chans[j].soundnum == num) n++;
	}
	return n;
}

int issoundplaying(int num)
{
	int j,n=0;
	
	if (!jfaud || !havewave) return 0;
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) continue;
		if (chans[j].soundnum == num) n++;
	}
	
	return n;
}

int xyzsound(short num, short i, long x, long y, long z)
{
	JFAudMixerChannel *chan;
	int r, global = 0;
	float gain = 1.0, pitch = 1.0;

	if (!jfaud || !havewave ||
	    num >= NUM_SOUNDS ||
	    ((soundm[num] & SOUNDM_PARENT) && ud.lockout) ||	// parental mode
	    SoundToggle == 0 ||
	    (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
	    (ps[myconnectindex].gm & MODE_MENU)
	   ) return -1;

	if (soundm[num] & SOUNDM_PLAYER) {
		sound(num);
		return 0;
	}
	
	if (soundm[num] & SOUNDM_DUKE) {
		// Duke speech, one at a time only
		int j;
		
		if (VoiceToggle == 0 ||
		    (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && ud.coop != 1)
		   ) return -1;

		for (j=NumVoices-1; j>=0; j--) {
			if (!chans[j].chan || chans[j].owner < 0) continue;
			if (soundm[ chans[j].soundnum ] & SOUNDM_DUKE) return -1;
		}
	}
	
	// XXX: here goes musicandsfx ranging. This will change the refdist.
	
	{
		int ps = soundps[num], pe = soundpe[num], cx;
		cx = labs(pe-ps);
		if (cx) {
			if (ps < pe) pitch = translatepitch(ps + rand()%cx);
			else pitch = translatepitch(pe + rand()%cx);
		} else pitch = translatepitch(ps);
	}
	
	//gain += soundvo[num];
	if (PN != MUSICANDSFX &&
		!cansee(ps[screenpeek].oposx,ps[screenpeek].oposy,ps[screenpeek].oposz-(24<<8),
		ps[screenpeek].cursectnum,SX,SY,SZ-(24<<8),SECT) )
        gain *= 1.0/32.0;
	
	switch(num)
	{
		case PIPEBOMB_EXPLODE:
		case LASERTRIP_EXPLODE:
		case RPG_EXPLODE:
			gain = 1.0;
			global = 1;
			if (sector[ps[screenpeek].cursectnum].lotag == 2) pitch -= translatepitch(1024);
			break;
		default:
			if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&SOUNDM_DUKE) == 0)
				pitch = translatepitch(-768);
			//if( sndist > 31444 && PN != MUSICANDSFX)
			//	return -1;
			break;
	}

/*
	// XXX: this is shit
	if( Sound[num].num > 0 && PN != MUSICANDSFX )
	{
		if( SoundOwner[num][0].i == i ) stopsound(num);
		else if( Sound[num].num > 1 ) stopsound(num);
		else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
	}
*/
	
	chan = jfaud->PlaySound(sounds[num], NULL, soundpr[num]);
	if (!chan) return -1;
	
	chan->SetGain(gain);
	chan->SetPitch(pitch);
	chan->SetLoop(soundm[num] & SOUNDM_LOOP);
	if (soundm[num] & SOUNDM_GLOBAL) global = 1;
	chan->SetFilter((soundm[num]&SOUNDM_NICE) ? JFAudMixerChannel::Filter4Point : JFAudMixerChannel::FilterNearest);
	
	if (PN == APLAYER && sprite[i].yvel == screenpeek) {
		chan->SetRolloff(0.0);
		chan->SetFollowListener(true);
		chan->SetPosition(0.0, 0.0, 0.0);
	} else {
		chan->SetRolloff(global ? 0.0 : 1.0);
		chan->SetFollowListener(false);
		chan->SetPosition((float)x/UNITSPERMETRE, (float)(-z>>4)/UNITSPERMETRE, (float)y/UNITSPERMETRE);
	}
	
	r = keephandle(chan, num, i);
	if (r >= 0) chan->SetStopCallback(stopcallback, r);
	chan->Play();
	
	return 0;
}

void sound(short num)
{
	JFAudMixerChannel *chan;
	int r;
	float pitch = 1.0;

	if (!jfaud || !havewave ||
	    num >= NUM_SOUNDS ||
	    SoundToggle == 0 ||
	    ((soundm[num] & SOUNDM_DUKE) && VoiceToggle == 0) ||
	    ((soundm[num] & SOUNDM_PARENT) && ud.lockout)	// parental mode
	   ) return;

	{
		int ps = soundps[num], pe = soundpe[num], cx;
		cx = labs(pe-ps);
		if (cx) {
			if (ps < pe) pitch = translatepitch(ps + rand()%cx);
			else pitch = translatepitch(pe + rand()%cx);
		} else pitch = translatepitch(ps);
	}
	
	chan = jfaud->PlaySound(sounds[num], NULL, soundpr[num]);
	if (!chan) return;

	chan->SetGain(1.0);
	chan->SetPitch(pitch);
	chan->SetLoop(soundm[num] & SOUNDM_LOOP);
	chan->SetRolloff(0.0);
	chan->SetFollowListener(true);
	chan->SetPosition(0.0, 0.0, 0.0);
	chan->SetFilter((soundm[num]&SOUNDM_NICE) ? JFAudMixerChannel::Filter4Point : JFAudMixerChannel::FilterNearest);

	r = keephandle(chan, num, -1);
	if (r >= 0) chan->SetStopCallback(stopcallback, r);
	chan->Play();
}

int spritesound(unsigned short num, short i)
{
	if (num >= NUM_SOUNDS) return -1;
	return xyzsound(num,i,SX,SY,SZ);
}

void stopsound(short num)
{
	int j;
	
	if (!jfaud || !havewave) return;
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].soundnum != num) continue;
		
		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
	}
}

void stopspritesound(short num, short i)
{
	int j;
	
	if (!jfaud || !havewave) return;
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].owner != i || chans[j].soundnum != num) continue;
		
		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
		return;
	}
}

void stopenvsound(short num, short i)
{
	int j;
	
	if (!jfaud || !havewave) return;
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].owner != i) continue;

		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
	}
}

void pan3dsound(void)
{
	JFAudMixer *mix;
	int j, global;
	short i;
	long cx, cy, cz, sx,sy,sz;
	short ca,cs;
	float gain;

	numenvsnds = 0;
	if (!jfaud || !havewave) return;
	mix = jfaud->GetWave();
	if (!mix) return;

	jfaud->AgeCache();
	
	if(ud.camerasprite == -1) {
		cx = ps[screenpeek].oposx;
		cy = ps[screenpeek].oposy;
		cz = ps[screenpeek].oposz;
		cs = ps[screenpeek].cursectnum;
		ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
	} else {
		cx = sprite[ud.camerasprite].x;
		cy = sprite[ud.camerasprite].y;
		cz = sprite[ud.camerasprite].z;
		cs = sprite[ud.camerasprite].sectnum;
		ca = sprite[ud.camerasprite].ang;
	}

	mix->SetListenerPosition((float)cx/UNITSPERMETRE, (float)(-cz>>4)/UNITSPERMETRE, (float)cy/UNITSPERMETRE);
	mix->SetListenerOrientation((float)sintable[(ca+512)&2047]/16384.0, 0.0, (float)sintable[ca&2047]/16384.0,
		0.0, 1.0, 0.0);
	
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].owner < 0) continue;

		global = 0;
		gain = 1.0;
		i = chans[j].owner;

		sx = sprite[i].x;
		sy = sprite[i].y;
		sz = sprite[i].z;

		//gain += soundvo[num];
		if (PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
			gain *= 1.0/32.0;
		
		if(PN == MUSICANDSFX && SLT < 999) numenvsnds++;
		if( soundm[ chans[j].soundnum ]&SOUNDM_GLOBAL ) global = 1;
		
		switch(chans[j].soundnum) {
			case PIPEBOMB_EXPLODE:
			case LASERTRIP_EXPLODE:
			case RPG_EXPLODE:
				gain = 1.0;
				global = 1;
				break;
			default:
				//if( sndist > 31444 && PN != MUSICANDSFX) {
				//	stopsound(j);
				//	continue;
				//}
				break;
		}
		
		// A sound may move from player-relative 3D if the viewpoint shifts from the player
		// through a viewscreen or viewpoint switching
		chans[j].chan->SetGain(gain);
		if (PN == APLAYER && sprite[i].yvel == screenpeek) {
			chans[j].chan->SetRolloff(0.0);
			chans[j].chan->SetFollowListener(true);
			chans[j].chan->SetPosition(0.0, 0.0, 0.0);
		} else {
			chans[j].chan->SetRolloff(global ? 0.0 : 1.0);
			chans[j].chan->SetFollowListener(false);
			chans[j].chan->SetPosition((float)sx/UNITSPERMETRE, (float)(-sz>>4)/UNITSPERMETRE, (float)sy/UNITSPERMETRE);
		}
	}
}

void clearsoundlocks(void)
{
}

void FX_SetVolume( int volume )
{
}

void FX_SetReverseStereo( int setting )
{
}

void FX_SetReverb( int reverb )
{
}

void FX_SetReverbDelay( int delay )
{
}

int FX_VoiceAvailable( int priority )
{
	int j;
	
	if (!jfaud) return 0;
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) return 1;
	}
	return 0;
}

int FX_PlayVOC3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	printf("FX_PlayVOC3D()\n");
	return 0;
}

int FX_PlayWAV3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	printf("FX_PlayWAV3D()\n");
	return 0;
}

int FX_StopSound( int handle )
{
	printf("FX_StopSound()\n");
	return 0;
}

int FX_StopAllSounds( void )
{
	int j;
	
	if (!jfaud || !havewave) return 0;
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) continue;

		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
	}

	return 0;
}

void MUSIC_SetVolume( int volume )
{
}

void MUSIC_Pause( void )
{
	if (jfaud) jfaud->PauseMusic(true);
}

void MUSIC_Continue( void )
{
	if (jfaud) jfaud->PauseMusic(false);
}

int MUSIC_StopSong( void )
{
	if (jfaud) jfaud->StopMusic();
	return 0;
}

void MUSIC_RegisterTimbreBank( unsigned char *timbres )
{
}

