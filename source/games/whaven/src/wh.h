#pragma once

#include "build.h"
#include "gamestruct.h"

BEGIN_WH_NS

using SPRITE = spritetype;
using WALL = walltype;
using boolean = bool;
using byte = uint8_t;

END_WH_NS

#include "globals.h"
#include "names.h"
#include "wh1names.h"
#include "wh2names.h"
#include "sndnames.h"
#include "player.h"
#include "ai.h"
#include "printf.h"
#include "gstrings.h"
#include "gamecontrol.h"
#include "d_net.h"
#include "screenjob.h"
#include "raze_sound.h"

BEGIN_WH_NS


enum EItems
{
	ITEMSBASE = 101,
	SILVERBAGTYPE = 101,
	GOLDBAGTYPE = 102,
	HELMETTYPE = 103,
	PLATEARMORTYPE = 104,
	CHAINMAILTYPE = 105,
	LEATHERARMORTYPE = 106,
	GIFTBOXTYPE = 107,
	FLASKBLUETYPE = 108,
	FLASKGREENTYPE = 109,
	FLASKOCHRETYPE = 110,
	FLASKREDTYPE = 111,
	FLASKTANTYPE = 112,
	DIAMONDRINGTYPE = 113,
	SHADOWAMULETTYPE = 114,
	GLASSSKULLTYPE = 115,
	AHNKTYPE = 116,
	BLUESCEPTERTYPE = 117,
	YELLOWSCEPTERTYPE = 118,
	ADAMANTINERINGTYPE = 119,
	ONYXRINGTYPE = 120,
	PENTAGRAMTYPE = 121,
	CRYSTALSTAFFTYPE = 122,
	AMULETOFTHEMISTTYPE = 123,
	HORNEDSKULLTYPE = 124,
	THEHORNTYPE = 125,
	SAPHIRERINGTYPE = 126,
	BRASSKEYTYPE = 127,
	BLACKKEYTYPE = 128,
	GLASSKEYTYPE = 129,
	IVORYKEYTYPE = 130,
	SCROLLSCARETYPE = 131,
	SCROLLNIGHTTYPE = 132,
	SCROLLFREEZETYPE = 133,
	SCROLLMAGICTYPE = 134,
	SCROLLOPENTYPE = 135,
	SCROLLFLYTYPE = 136,
	SCROLLFIREBALLTYPE = 137,
	SCROLLNUKETYPE = 138,
	QUIVERTYPE = 139,
	BOWTYPE = 140,
	WEAPON1TYPE = 141,
	WEAPON1ATYPE = 142,
	GOBWEAPONTYPE = 143,
	WEAPON2TYPE = 144,
	WEAPON3ATYPE = 145,
	WEAPON3TYPE = 146,
	WEAPON4TYPE = 147,
	THROWHALBERDTYPE = 148,
	WEAPON5TYPE = 149,
	GONZOSHIELDTYPE = 150,
	SHIELDTYPE = 151,
	WEAPON5BTYPE = 152,
	WALLPIKETYPE = 153,
	WEAPON6TYPE = 154,
	WEAPON7TYPE = 155,
	GYSERTYPE = 156, //WH1
	SPIKEBLADETYPE = 157,
	SPIKETYPE = 158,
	SPIKEPOLETYPE = 159,
	MONSTERBALLTYPE = 160,
	WEAPON8TYPE = 161,
	MAXITEMS = 162,
};

struct Point
{
	int x, y;
	int getX() const { return x; }
	int getY() const { return y; }
};


struct Delayitem {
     int  item;
     int  timer;
     boolean  func;

	 void memmove(const Delayitem& source)
	 {
		 item = source.item;
		 timer = source.timer;
		 func = source.func;
	 }

};

struct SwingDoor {
	int wall[8];
	int  sector;
	int  angopen;
	int  angclosed;
	int  angopendir;
	int  ang;
	int  anginc;
	int x[8];
	int y[8]; 
};

struct PLOCATION {
	int x;
	int y;
	int z;
	float ang;
	float horiz;
};

extern int ihaveflag;
extern int justplayed;
extern int lopoint;
extern int walktoggle;
extern int runningtime;
extern int oldhoriz;
extern int gNameShowTime;


extern int killcnt, kills;
extern int treasurescnt, treasuresfound;
extern int expgained;
extern int difficulty;
extern int lockclock;
extern SPRITE tspritelist[MAXSPRITESONSCREEN + 1];
extern int tspritelistcnt;
extern short arrowsprite[ARROWCOUNTLIMIT], throwpikesprite[THROWPIKELIMIT];
extern int sparksx, sparksy, sparksz;
extern int playertorch;
extern uint8_t ceilingshadearray[MAXSECTORS];
extern uint8_t floorshadearray[MAXSECTORS];
extern uint8_t wallshadearray[MAXWALLS];
extern short floormirrorsector[64];
extern int floormirrorcnt;
extern int displaytime;
extern int redcount, whitecount;

extern int zr_ceilz, zr_ceilhit, zr_florz, zr_florhit;
void getzrange(int x, int y, int z, short sectnum, int walldist, int cliptype);

struct Neartag {
	int taghitdist;
	short tagsector, tagwall, tagsprite;
};
void   neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, Neartag& nt, int32_t neartagrange, uint8_t tagsearch);

struct Hitscan {
	int hitx = -1, hity = -1, hitz = -1;
	short hitsect = -1, hitwall = -1, hitsprite = -1;
};

int hitscan(int xs, int ys, int zs, short sectnum, int vx, int vy, int vz, Hitscan& hit, int cliptype);
Point rotatepoint(int xpivot, int ypivot, int x, int y, short daang);

// whobj

extern int justwarpedcnt;
extern byte flashflag;
extern short torchpattern[];
extern int monsterwarptime;

void timerprocess(PLAYER& plr);
int getPickHeight();
void processobjs(PLAYER& plr);
void newstatus(short sn, int seq);
void makeafire(int i, int firetype);
void explosion(int i, int x, int y, int z, int owner);
void explosion2(int i, int x, int y, int z, int owner);
void trailingsmoke(int i, boolean ball);
void icecubes(int i, int x, int y, int z, int owner);
boolean damageactor(PLAYER& plr, int hitobject, short i);
int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, int cliptype);
void trowajavlin(int s);
void spawnhornskull(short i);
void spawnapentagram(int sn);


// whplr

extern PLAYER player[MAXPLAYERS];
extern PLOCATION gPrevPlayerLoc[MAXPLAYERS];
extern short monsterangle[MAXSPRITESONSCREEN], monsterlist[MAXSPRITESONSCREEN];
extern int shootgunzvel;
extern boolean justteleported;
extern int victor;
extern int autohoriz; // XXX NOT FOR MULTIPLAYER

extern int pyrn;
extern int mapon;
extern int damage_vel, damage_svel, damage_angvel;

void viewBackupPlayerLoc(int nPlayer);
void playerdead(PLAYER& plr);
void initplayersprite(PLAYER& plr);
void updateviewmap(PLAYER& plr);
void plruse(PLAYER& plr);
void chunksofmeat(PLAYER& plr, int hitsprite, int hitx, int hity, int hitz, short hitsect, int daang);
void addhealth(PLAYER& plr, int hp);
void addarmor(PLAYER& plr, int arm);
void addscore(PLAYER* plr, int score);
void goesupalevel(PLAYER& plr);
void lockon(PLAYER& plr, int numshots, int shootguntype);
void goesupalevel1(PLAYER& plr);
void goesupalevel2(PLAYER& plr);
void dophysics(PLAYER& plr, int goalz, int flyupdn, int v);

inline int getPlayerHeight()
{
	return isWh2() ? WH2PLAYERHEIGHT : PLAYERHEIGHT;
}


// whtag.cpp

extern int d_soundplayed;
extern int delaycnt;
extern Delayitem delayitem[MAXSECTORS];

extern short ironbarsector[16];
extern short ironbarscnt;
extern int ironbarsgoal1[16], ironbarsgoal2[16];
extern short ironbarsdone[16], ironbarsanim[16];
extern int ironbarsgoal[16];

extern short warpsectorlist[64], warpsectorcnt;
extern short xpanningsectorlist[16], xpanningsectorcnt;
extern short ypanningwalllist[128], ypanningwallcnt;
extern short floorpanninglist[64], floorpanningcnt;
extern SwingDoor swingdoor[MAXSWINGDOORS];
extern short swingcnt;

extern short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
extern int dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];


void operatesprite(PLAYER& plr, short s);
void operatesector(PLAYER& plr, int s);
void animatetags(int nPlayer);
void dodelayitems(int tics);
void setdelayfunc(int item, int delay);

// whmap

extern boolean nextlevel;

void preparesectors();
boolean prepareboard(const char* fname);

// whani
void animateobjs(PLAYER& plr);
boolean isBlades(int pic);

// weapons

extern WEAPONINF sspellbookanim[MAXNUMORBS][9];
extern WEAPONINF spikeanimtics[5];
extern WEAPONINF wh2throwanimtics[MAXNUMORBS][MAXFRAMES + 1];
extern WEAPONINF throwanimtics[MAXNUMORBS][MAXFRAMES + 1];
extern WEAPONINF cockanimtics[MAXFRAMES + 1];
extern WEAPONINF zcockanimtics[MAXFRAMES + 1];
extern WEAPONINF zreadyanimtics[MAXWEAPONS][MAXFRAMES + 1];
extern WEAPONINF readyanimtics[MAXWEAPONS][MAXFRAMES + 1];
extern WEAPONINF weaponanimtics[MAXWEAPONS][MAXFRAMES];
extern WEAPONINF zweaponanimtics[MAXWEAPONS][MAXFRAMES];
extern WEAPONINF zlefthandanimtics[5][MAXFRAMES];
extern WEAPONINF weaponanimtics2[MAXWEAPONS][MAXFRAMES];
extern WEAPONINF zweaponanimtics2[MAXWEAPONS][MAXFRAMES];
extern WEAPONINF lefthandanimtics[5][MAXFRAMES];

extern int dropshieldcnt;
extern boolean droptheshield;
extern int dahand;
extern int weapondrop;
extern int snakex, snakey;

boolean checkmedusadist(int i, int x, int y, int z, int lvl);
void autoweaponchange(PLAYER& plr, int dagun);
void weaponchange(int snum);
void plrfireweapon(PLAYER& plr);
void weaponsprocess(int snum);
void shootgun(PLAYER& plr, float ang, int guntype);
boolean checkweapondist(int i, int x, int y, int z, int guntype);
void swingdapunch(PLAYER& plr, int daweapon);
void swingdaweapon(PLAYER& plr);
void swingdacrunch(PLAYER& plr, int daweapon);
void swingdasound(int daweapon, boolean enchanted);

boolean isItemSprite(int i);
void InitItems();

// spellbook

void activatedaorb(PLAYER& plr);
void castaorb(PLAYER& plr);
void spellswitch(PLAYER& plr, int j);
void bookprocess(int snum);
boolean changebook(PLAYER& plr, int i);
boolean lvlspellcheck(PLAYER& plr);
void speelbookprocess(PLAYER& plr);
void nukespell(PLAYER& plr, short j);
void medusa(PLAYER& plr, short j);
void displayspelltext(PLAYER& plr);
void orbpic(PLAYER& plr, int currentorb);

// potion
extern int potiontilenum;
void potiontext(PLAYER& plr);
void potionchange(int snum);
void usapotion(PLAYER& plr);
boolean potionspace(PLAYER& plr, int vial);
void updatepotion(PLAYER& plr, int vial);
void potionpic(PLAYER& plr, int currentpotion, int x, int y, int scale);
void randompotion(int i);

// whfx

extern short skypanlist[64], skypancnt;
extern short lavadrylandsector[32];
extern short lavadrylandcnt;
extern short bobbingsectorlist[16], bobbingsectorcnt;
extern int justwarpedfx;
extern int lastbat;
extern short revolveclip[16];
extern short revolvesector[4], revolveang[4], revolvecnt;
extern int revolvex[4][32], revolvey[4][32];
extern int revolvepivotx[4], revolvepivoty[4];
extern int warpx, warpy, warpz, warpang;
extern short warpsect;
extern int scarytime;
extern int scarysize;
extern int thunderflash;
extern int thundertime;


void initlava();
void movelava();
void initwater();
void movewater();
void skypanfx();
void panningfx();
void revolvefx();
void bobbingsector();
void teleporter();
void warp(int x, int y, int z, int daang, short dasector);
void warpsprite(short spritenum);
void ironbars();
void sectorsounds();
void scaryprocess();
void dofx();
void thunder();
void thesplash();
void makeasplash(int picnum, PLAYER& plr);
void makemonstersplash(int picnum, int i);
void bats(PLAYER& plr, int k);
void cracks();
void lavadryland();
void warpfxsprite(int s);
void FadeInit();
void resetEffects();
void weaponpowerup(PLAYER& plr);
void makesparks(short i, int type);
void shards(int i, int type);


// animate

struct Loc
{
	int x, y, z, ang;
};
extern Loc oldLoc[MAXSPRITES];

struct ANIMATION
{
	short id;
	byte type;
	int goal;
	int vel;
	int acc;
};

extern ANIMATION gAnimationData[MAXANIMATES];
extern int gAnimationCount;

int getanimationgoal(sectortype& object, int type);
int setanimation(int index, int thegoal, int thevel, int theacc, int type);
void doanimations();



inline void showmessage(const char* msg, int)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings(msg));
}

inline bool isValidSector(int num)
{
	return ((unsigned)num < numsectors);
}

inline int BClampAngle(int a)
{
	return a & 2047;
}


// placeholders 

// This is for the 3 sounds that get explicitly checked outside the sound code.
enum
{
	CHAN_ENCHANTED = 100,
	CHAN_CART,
	CHAN_BAT,
	CHAN_AMBIENT1,
	CHAN_AMBIENT2,
	CHAN_AMBIENT3,
	CHAN_AMBIENT4,
	CHAN_AMBIENT5,
	CHAN_AMBIENT6,
	CHAN_AMBIENT7,
	CHAN_AMBIENT8,
};

enum
{
	MAX_AMB_SOUNDS = 8,
};

extern int ambsoundarray[8];

void startredflash(int);
void startwhiteflash(int);
void startgreenflash(int);
void startblueflash(int);
void updatepaletteshifts();
void resetflash();
void applyflash();


void updatesounds();
int playsound_internal(int sn, spritetype* spr, int x, int y, int loop, int chan);

inline int playsound(int sn, int x, int y, int loop = 0, int channel = CHAN_AUTO) {
	return playsound_internal(sn, nullptr, x, y, loop, channel);
}

inline int SND_Sound(int sn) {
	return playsound(sn, 0, 0);
}

inline int spritesound(int sn, spritetype *s, int loop = 0, int channel = CHAN_AUTO) {
	return playsound_internal(sn, s, 0, 0, loop, channel);
}

void startmusic(int);
void startsong(int);
void setupmidi();

extern int attacktheme;

inline int insertsprite(int sectnum, int statnum)
{
	int j = ::insertsprite(sectnum, statnum);
	if (j != -1)
		sprite[j].detail = 0;
	return j;
}

void analyzesprites(PLAYER& plr, int dasmoothratio);
void precacheTiles();


void startWh2Ending(CompletionFunc);
void showStatisticsScreen(CompletionFunc);
void showVictoryScreen(CompletionFunc);

void InitNames();
void InitFonts();
void sfxInit(void);

void IntroMovie(const CompletionFunc& completion);


#include "item.h"


struct GameInterface : public ::GameInterface
{
	const char* Name() override { return "Witchaven"; }
	void app_init() override;
	//void clearlocalinputstate() override;
	//bool GenerateSavePic() override;
	//void PlayHudSound() override;
	//GameStats getStats() override;
	//void MenuOpened() override;
	void MenuSound(EMenuSounds snd) override;
	bool CanSave() override;
	//bool StartGame(FNewGameStartup& gs) override;
	//FSavegameInfo GetSaveSig() override;
	void SerializeGameState(FSerializer& arc) override;
	//void QuitToTitle() override;
	FString GetCoordString() override;
	//void ExitFromMenu() override;
	//ReservedSpace GetReservedScreenSpace(int viewsize) override;
	//void DrawPlayerSprite(const DVector2& origin, bool onteam) override;
	//void GetInput(InputPacket* packet, ControlInfo* const hidInput) override;
	//void UpdateSounds() override;
	void Startup() override;
	void DrawBackground() override;
	//void Render() override;
	void Ticker() override;
	const char* GenericCheat(int player, int cheat) override;
	const char* CheckCheatMode() override;
	void NextLevel(MapRecord* map, int skill) override;
	void NewGame(MapRecord* map, int skill) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	//bool DrawAutomapPlayer(int x, int y, int z, int a) override;
	//int playerKeyMove() override { return 40; }

};
 
END_WH_NS

