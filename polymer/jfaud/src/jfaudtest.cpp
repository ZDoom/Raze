#ifdef __APPLE__
# include "jfaud/sysdefs.h"
#else
# include "sysdefs.h"
#endif

#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
# include "watcomhax/cstdio"
# include "watcomhax/cstring"
# include "watcomhax/cmath"
# include "watcomhax/ctime"
#else
# include <cstdlib>
# include <cstdio>
# include <cstring>
# include <cmath>
# include <ctime>
#endif

#define _SDL_audio_h
#ifdef __APPLE__
# include <SDL/SDL.h>
# include <SDL/SDL_main.h>
# include "jfaud/jfaud.hpp"
#else
# include "SDL.h"
# include "jfaud.hpp"
#endif


#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#define XRES 640
#define YRES 480

#define OSC 128
#define NCHANS 3

SDL_Surface *surf;
int counter = 0;

void ClearScreen(unsigned char c);
void DrawGrid(unsigned char c);
void DrawPixel(int x, int y, unsigned char c);
void DrawLine(int x0, int y0, int x1, int y1, unsigned char c);
void DrawOrigin(float x, float y, float c, float s);
void DrawWander(float x, float y, unsigned char c);
void DrawSource(float x, float y, unsigned char c);

typedef struct _ChanListItem {
	struct _ChanListItem *next;
	JFAudMixerChannel * it;
	int tag;
} ChanListItem;
class ChanList {
private:
	ChanListItem *items, *cur;
public:
	ChanList() : items(NULL), cur(NULL) { }
	~ChanList() {
		ChanListItem *x;
		while (items) {
			x = items->next;
			delete items;
			items = x;
		}
	}
	void Store(JFAudMixerChannel * x, int t = 0) {
		ChanListItem *y;
		y = items;
		while (y) {
			if (y->it == x) {
				y->tag = t;
				return;
			}
			y = y->next;
		}
		y = new ChanListItem;
		y->it = x;
		y->tag = t;
		y->next = items;
		items = y;
	}
	void Rewind() { cur = items; }
	bool Each(JFAudMixerChannel * *x, int *tag = NULL) {
		if (!cur) return false;
		*x = cur->it;
		if (tag) *tag = cur->tag;
		Next();
		return true;
	}
	void Next() {
		if (!cur) return;
		cur = cur->next;
	}
};

/*
#include "soundcache.hpp"
#include "memfile.hpp"
int testcache(void)
{
	SoundCache *cache;
	MemFile *testfile;
	JFAudFile *cachef;
	void *xbuf;

	cache = new SoundCache();
	cache->SetCacheSize(262144, 262144);

	xbuf = malloc(262144);
	testfile = new MemFile(xbuf, 262144, free);

	printf("caching 262144 byte file\n");
	cachef = cache->CacheFile(static_cast<JFAudFile*>(testfile), "bigfile",NULL);
	cache->Update();

	printf("releasing 262144 byte file\n");
	if (cachef) delete cachef;
	cache->Update();

	xbuf = malloc(65535);
	testfile = new MemFile(xbuf, 65535, free);

	printf("caching 65535 byte file\n");
	cachef = cache->CacheFile(static_cast<JFAudFile*>(testfile), "smallfile",NULL);
	cache->Update();

	printf("releasing 65535 byte file\n");
	if (cachef) delete cachef;
	cache->Update();

	delete cache;

	return 0;
}
*/

extern "C" int main(int argc, char *argv[])
{
	char **enumer, **enumer2, *def;
	int i,j;
	bool done = false;
	SDL_Event ev;
	char buttons=0;

	char *clicksound  = "testsnd/pop.wav";
	char *wandersound = "testsnd/relaxation.au";
	char *bgmsound    = "testsnd/fishpolk.mid";
	char *wavedev     = NULL;
	char *mididev     = NULL;
	char *cdadev      = NULL;

	bool nowave=false, nomidi=false, nocda=false;

	float originx=0.0, originz=0.0, origina=-M_PI/2.0;
	float mousex=0.0, mousez=0.0;

	JFAud *jfaud = NULL;
	JFAudMixerChannel *wanderchan = NULL, *ch;
	ChanList clicks;
	bool paused = false;

	srand((unsigned int)time(NULL));

	for (i=1; i<argc; i++) {
		     if (!strcasecmp(argv[i], "-wave"))   wavedev = argv[++i];
		else if (!strcasecmp(argv[i], "-midi"))   mididev = argv[++i];
		else if (!strcasecmp(argv[i], "-cda"))    cdadev  = argv[++i];
		else if (!strcasecmp(argv[i], "-wander")) wandersound = argv[++i];
		else if (!strcasecmp(argv[i], "-click"))  clicksound  = argv[++i];
		else if (!strcasecmp(argv[i], "-bgm"))    bgmsound    = argv[++i];
		//else if (!strcasecmp(argv[i], "-testcache")) return testcache();
		else if (!strcasecmp(argv[i], "-help") || !strcasecmp(argv[i], "-?")) {
			printf("%s [options]\n"
				" Options:\n"
		//		"  -testcache           Perform a test of the cache code\n"
				"  -wave <device>	Use named wave device\n"
				"  -midi <device>	Use named MIDI device\n"
				"  -cda  <device>	Use named CDA device\n"
				"  -wander <fn>		Filename of the sound that wanders around the listener\n"
				"				default: %s\n"
				"  -click <fn>   	Filename of the sound that plays if you click the mouse\n"
				"				default: %s\n"
				"  -bgm <fn>		Filename of the background music sound (or *CDA[:track])\n"
				"				default: %s\n"
				"  -help, -?		This message\n"
				,argv[0],
				wandersound, clicksound, bgmsound);
			return 0;
		}
	}
	
	enumer = JFAud::EnumerateWaveDevices(NULL, &def);
	if (enumer) {
		printf("Wave devices\n  * default: %s\n", def);
		for (i=0; enumer[i]; i++) {
			printf("  - %s\n", enumer[i]);

			enumer2 = jfaud->EnumerateWaveDevices(enumer[i], &def);
			if (enumer2) {
				printf("    * default: %s\n", def);
				for (j=0; enumer2[j]; j++) {
					printf("    - %s\n", enumer2[j]);
				}
				free(enumer2);
			}
		}
		free(enumer);
	}

	enumer = JFAud::EnumerateMIDIDevices(&def);
	if (enumer) {
		printf("MIDI devices\n  * default: %s\n", def);
		for (i=0; enumer[i]; i++) {
			printf("  - %s\n", enumer[i]);
		}
		free(enumer);
	}

	enumer = JFAud::EnumerateCDADevices(&def);
	if (enumer) {
		printf("CDA devices\n  * default: %s\n", def);
		for (i=0; enumer[i]; i++) {
			printf("  - %s\n", enumer[i]);
		}
		free(enumer);
	}

	jfaud = new JFAud();

	// Initialising SDL here so the window is created before JFAud needs it
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)) {
		delete jfaud;
		puts("Error initialising SDL");
		return -1;
	}
	SDL_WM_SetCaption("JFAud Test", NULL);

	surf = SDL_SetVideoMode(XRES,YRES,8,SDL_SWSURFACE);
	if (!surf) {
		delete jfaud;
		SDL_Quit();
		puts("Error setting video mode");
		return -1;
	} else {
		SDL_Color cmap[256];
		int i;
		for (i=0;i<256;i++) {
			cmap[i].r = i;
			cmap[i].g = (cmap[i].r+32)%256;
			cmap[i].b = (cmap[i].g+64)%256;
		}
		SDL_SetColors(surf, cmap, 0, 256);
	}

	SDL_GetMouseState(NULL,NULL);
	SDL_EnableKeyRepeat(10,10);

	if (!jfaud->InitWave(wavedev, NCHANS)) {
		puts("Error initialising wave output");
		nowave = true;
	} else jfaud->SetCacheSize(1048576,1048576);

	if (!jfaud->InitMIDI(mididev)) {
		puts("Error initialising MIDI device. No biggie.");
		nomidi = true;
	}

	if (!strncasecmp(bgmsound, "*CDA", 4)) {
		if (!jfaud->InitCDA(cdadev)) {
			puts("Couldn't initialise CDA. No biggie.");
		} else {
			JFAudCDA *cda = jfaud->GetCDA();
			int rtrack = 0;

			if (bgmsound[4] == ':') {
				rtrack = atoi(bgmsound+5);
				if (!cda->IsTrackPlayable(rtrack))
					printf("CDA track %d isn't playable\n", rtrack);
				else {
					cda->PlayTrack(rtrack);
					printf("Playing CDA track %d\n", rtrack);
				}
			} else {
				int attempts = 3;
				if (cda->GetNumTracks() > 0) {
					do {
						rtrack = rand() % cda->GetNumTracks();
					} while (!cda->IsTrackPlayable(rtrack) && --attempts > 0);
				}
				if (attempts > 0) {
					printf("Playing CDA track %d\n", rtrack);
					cda->PlayTrack(rtrack);
				} else {
					printf("Gave up trying to find a playable CDA track\n");
					nocda = true;
				}
			}
		}
		bgmsound = NULL;
	} else {
		nocda = true;
	}

	if (wandersound && !nowave) {
		wanderchan = jfaud->PlaySound(wandersound, NULL, 2);
		if (!wanderchan) printf("Error playing wandering sound %s\n", wandersound);
		else {
			wanderchan->SetLoop(true);
			wanderchan->Play();
		}
	}

	if (bgmsound) {
		jfaud->PlayMusic(bgmsound);
	}

	while (1) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) { done = true; }
			else if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_SPACE: jfaud->PauseMusic(paused ^= true); break;
					case SDLK_ESCAPE: done = true; break;
					case SDLK_LEFT: origina -= M_PI/20.0; break;
					case SDLK_RIGHT: origina += M_PI/20.0; break;
					case SDLK_UP:
						originx += cos(origina)*0.1;
						originz += sin(origina)*0.1;
						break;
					case SDLK_DOWN:
						originx -= cos(origina)*0.1;
						originz -= sin(origina)*0.1;
						break;
					default: break;
				}
			} else if (ev.type == SDL_MOUSEBUTTONDOWN) {
				if (ev.button.button == SDL_BUTTON_LEFT) buttons |= 1;
			} else if (ev.type == SDL_MOUSEBUTTONUP) {
				if (ev.button.button == SDL_BUTTON_LEFT) buttons &= ~1;
			} else if (ev.type == SDL_MOUSEMOTION) {
				mousex = (float)(ev.motion.x-XRES/2) / OSC;
				mousez = (float)(ev.motion.y-YRES/2) / OSC;
			}
		}
		if (done) break;

		if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
		ClearScreen(0);
		DrawGrid(32);

		{
			float x,y,z;
			
			//DrawSource(mousex, mousez, 130);
			
			if (!nowave) {
				jfaud->GetWave()->SetListenerPosition(originx,0.0,originz);
				jfaud->GetWave()->SetListenerOrientation(cos(origina),0.0,sin(origina),0.0,1.0,0.0);
				DrawOrigin(originx,originz,cos(origina),sin(origina));
			}

			if (wanderchan) {
				y = M_PI*(float)counter/180.0;
				x = cos(y);
				z = sin(y);
				DrawWander(x, z, 96);
				if (!paused) wanderchan->SetPosition(x,0.0,z);
			}

			if (buttons & 1) {
				ch = jfaud->PlaySound(clicksound, NULL, 1);
				if (!ch) clicksound = NULL;
				else {
					ch->SetFilter(JFAudMixerChannel::Filter4Point);
					ch->SetRefDist(1.0);
					ch->SetMaxDist(2.0);
					ch->SetRolloff(1.0);
					ch->SetPosition(mousex,0.0,mousez);
					ch->Play();
					clicks.Store(ch,0);
				}
				buttons &= ~1;
			}

			clicks.Rewind();
			while (clicks.Each(&ch,NULL)) {
				if (!jfaud->IsValidSound(ch) || !ch->IsPlaying()) { continue; }
				ch->GetPosition(&x,&y,&z);
				DrawSource(x, z, 200);
			}
		}
		
		if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);

		SDL_Flip(surf);

		jfaud->Update();

		SDL_Delay(50);
		if (!paused) counter++;
	}

	SDL_Quit();

	if (wanderchan) jfaud->FreeSound(wanderchan);
	clicks.Rewind(); while (clicks.Each(&ch,NULL)) jfaud->FreeSound(ch);
	delete jfaud;
	
	return 0;
}

void ClearScreen(unsigned char c)
{
	memset(surf->pixels, c, YRES * surf->pitch);
}

void DrawGrid(unsigned char c)
{
	int i;
	DrawLine(0,YRES/2,XRES-1,YRES/2,c);
	DrawLine(XRES-1-16, YRES/2-16, XRES-1, YRES/2, c);
	DrawLine(XRES-1-16, YRES/2+16, XRES-1, YRES/2, c);
	DrawLine(XRES/2,0,XRES/2,YRES-1,c);
	DrawLine(XRES/2-16, YRES-1-16, XRES/2, YRES-1, c);
	DrawLine(XRES/2+16, YRES-1-16, XRES/2, YRES-1, c);

	for (i = (XRES/2)/OSC; i>=0; i--) {
		DrawLine(XRES/2+i*OSC,YRES/2-8,XRES/2+i*OSC,YRES/2+8, c);
		DrawLine(XRES/2-i*OSC,YRES/2-8,XRES/2-i*OSC,YRES/2+8, c);
	}
	for (i = (YRES/2)/OSC; i>=0; i--) {
		DrawLine(XRES/2-8,YRES/2+i*OSC,XRES/2+8,YRES/2+i*OSC, c);
		DrawLine(XRES/2-8,YRES/2-i*OSC,XRES/2+8,YRES/2-i*OSC, c);
	}
}

void DrawPixel(int x, int y, unsigned char c)
{
	if (x < 0 || y < 0 || x >= XRES || y >= YRES) return;
	((unsigned char *)surf->pixels)[(y * surf->pitch) + x] = c;
}

void DrawLine(int x0, int y0, int x1, int y1, unsigned char c)
{
	int dx, dy, x, y, steps;
	dx = (x1-x0);
	dy = (y1-y0);
	steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
	DrawPixel(x0,y0,c);
	if (steps == 0) return;
	x = x0<<8;
	y = y0<<8;
	dx = (dx<<8) / steps;
	dy = (dy<<8) / steps;
	for (--steps ; steps >= 0; steps--) {
		x += dx;
		y += dy;
		DrawPixel(x>>8,y>>8,c);
	}
}

void RotatePoint(float *x, float *y, float c, float s)
{
	float nx,ny;
	nx = c*(*x) - s*(*y);
	ny = s*(*x) + c*(*y);
	*x = nx; *y = ny;
}

void DrawOrigin(float x, float y, float c, float s)
{
	float ox,oy, tx,ty, lx,ly, rx,ry;

	ox = XRES/2 + x*OSC; oy = YRES/2 + y*OSC;
	tx = 24.0; ty =  0.0; RotatePoint(&tx, &ty, c,s);
	lx = 0.0;  ly =  8.0; RotatePoint(&lx, &ly, c,s);
	rx = 0.0;  ry = -8.0; RotatePoint(&rx, &ry, c,s);

	DrawLine((int)(ox+tx),(int)(oy+ty),(int)(ox+lx),(int)(oy+ly), 255);
	DrawLine((int)(ox+tx),(int)(oy+ty),(int)(ox+rx),(int)(oy+ry), 255);
	DrawLine((int)(ox+lx),(int)(oy+ly),(int)(ox+rx),(int)(oy+ry), 255);
}

void DrawSource(float x, float y, unsigned char c)
{
	int l,t,r,b,m,n;
	m = XRES/2 + (int)(x*OSC); n = YRES/2 + (int)(y*OSC);
	l = XRES/2 + (int)(x*OSC) - 8; t = YRES/2 + (int)(y*OSC) - 8;
	r = XRES/2 + (int)(x*OSC) + 8; b = YRES/2 + (int)(y*OSC) + 8;
	DrawLine(m,t,l,n, c);
	DrawLine(m,t,r,n, c);
	DrawLine(l,n,m,b, c);
	DrawLine(r,n,m,b, c);
}

void DrawWander(float x, float y, unsigned char c)
{
	int l,t,r,b;
	l = XRES/2 + (int)(x*OSC) - 8; t = YRES/2 + (int)(y*OSC) - 8;
	r = XRES/2 + (int)(x*OSC) + 8; b = YRES/2 + (int)(y*OSC) + 8;
	DrawLine(l,t,r,t, c);
	DrawLine(l,b,r,b, c);
	DrawLine(l,t,l,b, c);
	DrawLine(r,t,r,b, c);
}

