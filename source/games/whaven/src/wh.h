#pragma once

#include "build.h"

BEGIN_WH_NS

using SPRITE = spritetype;
using boolean = bool;
using byte = uint8_t;

END_WH_NS

#include "globals.h"
#include "names.h"
#include "item.h"
#include "wh1names.h"
#include "wh2names.h"
#include "sndnames.h"
#include "player.h"
#include "ai.h"
#include "printf.h"
#include "gstrings.h"
#include "gamecontrol.h"

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


extern int killcnt;

// whobj

extern int justwarpedcnt;
extern byte flashflag;
extern short torchpattern[];
extern int monsterwarptime;

short adjusthp(int hp);
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
void addscore(PLAYER& plr, int score);
void goesupalevel(PLAYER& plr);

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

void loadnewlevel(int mapon);
void preparesectors();
boolean prepareboard(const char* fname);

// whani
void animateobjs(PLAYER& plr);
boolean isBlades(int pic);

// weapons

extern const WEAPONINF sspellbookanim[MAXNUMORBS][9];
extern const WEAPONINF spikeanimtics[5];
extern const WEAPONINF wh2throwanimtics[MAXNUMORBS][MAXFRAMES + 1];
extern const WEAPONINF throwanimtics[MAXNUMORBS][MAXFRAMES + 1];
extern const WEAPONINF cockanimtics[MAXFRAMES + 1];
extern const WEAPONINF zcockanimtics[MAXFRAMES + 1];
extern const WEAPONINF zreadyanimtics[MAXWEAPONS][MAXFRAMES + 1];
extern const WEAPONINF readyanimtics[MAXWEAPONS][MAXFRAMES + 1];
extern const WEAPONINF weaponanimtics[MAXWEAPONS][MAXFRAMES];
extern const WEAPONINF zweaponanimtics[MAXWEAPONS][MAXFRAMES];
extern const WEAPONINF zlefthandanimtics[5][MAXFRAMES];
extern const WEAPONINF weaponanimtics2[MAXWEAPONS][MAXFRAMES];
extern const WEAPONINF zweaponanimtics2[MAXWEAPONS][MAXFRAMES];
extern const WEAPONINF lefthandanimtics[5][MAXFRAMES];

extern int dropshieldcnt;
extern boolean droptheshield;
extern int dahand;
extern int weapondrop;
extern int snakex, snakey;
extern int enchantedsoundhandle;

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


inline void showmessage(const char* msg, int)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings(msg));
}

END_WH_NS
