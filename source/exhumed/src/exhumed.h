#ifndef __exhumed_h__
#define __exhumed_h__

#include "compat.h"
#include "cache1d.h"

#define kTimerTicks		120

#ifdef __WATCOMC__
void handleevents();
#endif

#ifndef APPNAME
#define APPNAME             "Exhumed"
#endif

#ifndef APPBASENAME
#define APPBASENAME         "exhumed"
#endif

// TODO:
#define OSDTEXT_DEFAULT   "^00"
#define OSDTEXT_DARKRED   "^00"
#define OSDTEXT_GREEN     "^00"
#define OSDTEXT_RED       "^00"
#define OSDTEXT_YELLOW    "^00"

#define OSDTEXT_BRIGHT    "^S0"

#define OSD_ERROR OSDTEXT_DARKRED OSDTEXT_BRIGHT

void ShutDown(void);
void DebugOut(const char *fmt, ...);
void bail2dos(const char *fmt, ...);
int ExhumedMain(int argc, char *argv[]);

void FinishLevel();

void SetHiRes();

void BlackOut();

void DoGameOverScene();

int Query(short n, short l, ...);

extern unsigned char curpal[];

void TintPalette(int a, int b, int c);
void MySetPalette(unsigned char *palette);
void GetCurPal(unsigned char *palette);

void EraseScreen(int eax);

void RestorePalette();

int FindGString(const char *str);

void WaitTicks(int nTicks);

void FadeIn();
void FadeOut(int bFadeMusic);

int myprintext(int x, int y, const char *str, int shade);
int MyGetStringWidth(const char *str);

void mychangespritesect(int nSprite, int nSector);
void mydeletesprite(int nSprite);

void GrabPalette();

void mysetbrightness(char nBrightness);

void StartFadeIn();
int DoFadeIn();

void InitSpiritHead();

int CopyCharToBitmap(char nChar, int nTile, int xPos, int yPos);

// TODO - relocate
void StatusMessage(int messageTime, const char *fmt, ...);

int DoSpiritHead();

void UpdateScreenSize();

extern buildvfs_kfd kopen4loadfrommod(const char* filename, char searchfirst);
extern int32_t g_commandSetup;
extern int32_t g_noSetup;

extern short nCDTrackLength;

extern char sHollyStr[];

extern int localclock;

extern int moveframes;

extern short bSerialPlay;

extern int nNetPlayerCount;

extern int htimer;

extern int nNetTime;
extern short barpages;

extern short nTotalPlayers;

extern short nFontFirstChar;
extern short nBackgroundPic;
extern short nShadowPic;

extern short nCreaturesLeft;

extern int lLocalButtons;

extern short nEnergyTowers;

extern short nEnergyChan;

extern short nSpiritSprite;

extern short bNoCDCheck;

extern short bInDemo;

extern short nFreeze;

extern short nCurBodyNum;
extern short nBodyTotal;

extern short bSnakeCam;

extern short levelnum;
//extern short nScreenWidth;
//extern short nScreenHeight;

extern short bMapMode;

extern short nButtonColor;

extern short nHeadStage;

extern short lastfps;

extern int flash;

extern short bNoCreatures;

extern short nLocalSpr;
extern short levelnew;

extern short textpages;

extern short nSnakeCam;

extern short bHiRes;
extern short bCoordinates;
extern short bFullScreen;

extern short bHolly;

extern short screensize;

extern int totalmoves;

extern short nGamma;

extern short screenpage;

extern int lCountDown;

extern short bSlipMode;

extern short nItemTextIndex;
extern const char *gString[];

extern char g_modDir[BMAX_PATH];
enum {
    kPalNormal = 0,
    kPalNoDim,
    kPalTorch,
    kPalNoTorch,
    kPalBrite,
    kPalRedBrite,
    kPalGreenBrite,
    kPalNormal2,
    kPalNoDim2,
    kPalTorch2,
    kPalNoTorch2,
    kPalBrite2
};

#endif
