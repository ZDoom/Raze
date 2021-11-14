#pragma once
#include "wh.h"

BEGIN_WH_NS

short adjusthp(int hp);

struct EnemyInfo 
{
	short sizx, sizy;
	int attackdist;
	int attackdamage;
	int attackheight;
	short health;
	bool fly;
	int clipdist;
	int score;
	
	void Init(int sizx, int sizy, int dist, int height, int damage, int clipdist, bool fly, int health, int score)
	{
		this->sizx = (short) sizx;
		this->sizy = (short) sizy;
		this->attackdist = dist;
		this->attackheight = height;
		this->attackdamage = damage;
		this->clipdist = clipdist;
		this->fly = fly;
		this->health = (short) health;
		this->score = score;
	}
	
	short (*getHealth)(EnemyInfo& e, SPRITE& spr) = [](EnemyInfo& e, SPRITE& spr)
	{
		return adjusthp(e.health);
	};
	
	int (*getAttackDist)(EnemyInfo& e, SPRITE& spr) = [](EnemyInfo &e, SPRITE& spr)
	{
		return e.attackdist;
	};
	
	void set(SPRITE &spr )
	{
		spr.clipdist = clipdist;
		spr.hitag = getHealth(*this, spr);
		if(sizx != -1)
			spr.xrepeat = (uint8_t)sizx;
		if(sizy != -1)
			spr.yrepeat = (uint8_t)sizy;
		spr.lotag = 100;
		
		int tflag = 0;
		if((spr.cstat & 514) != 0) 
			tflag = spr.cstat & 514;

		spr.cstat = (short) (0x101 | tflag);
	}
};


using AIState = void (*)(PLAYER &plr, DWHActor* actor);


struct Enemy 
{
	EnemyInfo info;
	
	AIState patrol;
	AIState chase;
	AIState resurect;
	AIState nuked;
	AIState frozen;
	AIState pain;
	AIState face;
	AIState attack;
	AIState flee;
	AIState cast;
	AIState die;
	AIState skirmish;
	AIState stand;
	AIState search;
};


enum EEnemy
{
	DEMONTYPE = 1, // ok
	DEVILTYPE = 2, // nuked ok, frozen no
	DRAGONTYPE = 3, // wh1
	FATWITCHTYPE = 4, // wh1
	FISHTYPE = 5,
	FREDTYPE = 6,
	GOBLINTYPE = 7, // wh1
	GONZOTYPE = 8, // freeze nuke ok
	GRONTYPE = 9, // ok
	GUARDIANTYPE = 10, // nuke ok
	IMPTYPE = 11, // freeze nuke ok
	JUDYTYPE = 12, // wh1
	KATIETYPE = 13, // ok
	KOBOLDTYPE = 14, // freeze nuke ok
	KURTTYPE = 15,
	MINOTAURTYPE = 16, // freeze nuke ok
	NEWGUYTYPE = 17, // freeze nuke ok
	RATTYPE = 18,
	SKELETONTYPE = 19, // freezee nuke ok
	SKULLYTYPE = 20, // wh1
	SPIDERTYPE = 21, // wh1
	WILLOWTYPE = 22, //nuke ok
	MAXTYPES = 23,
};

enum EAIConst
{
	TYPENONE = 0,
	TYPEWATER = 1,
	TYPELAVA = 2,
};

extern Enemy enemy[MAXTYPES];
extern int checksight_ang;

void aiProcess();
Collision aimove(DWHActor* actor);
Collision aifly(DWHActor* i);
void aisearch(PLAYER& plr, DWHActor* i, boolean fly);
boolean checksector6(DWHActor* i);
int checkfluid(DWHActor* i, Collision& florhit);
void processfluid(DWHActor* i, Collision& florHit, boolean fly);
void castspell(PLAYER& plr, DWHActor* i);
void skullycastspell(PLAYER& plr, int i);
void attack(PLAYER& plr, DWHActor* i);
int checkmove(DWHActor* actor, int dax, int day);
boolean checkdist(PLAYER& plr, DWHActor* i);
boolean checkdist(DWHActor* i, int x, int y, int z);
extern int checksight_ang;
boolean checksight(PLAYER& plr, DWHActor* i);
void monsterweapon(int i);
PLAYER* aiGetPlayerTarget(short i);
boolean actoruse(short i);

void initAI();
void aiInit();

void premapDemon(DWHActor* i);
void premapDevil(DWHActor* i);
void premapDragon(DWHActor* i);
void premapFatwitch(DWHActor* i);
void premapFish(DWHActor* i);
void premapFred(DWHActor* i);
void premapGoblin(DWHActor* i);
void premapGonzo(DWHActor* i);
void premapGron(DWHActor* i);
void premapGuardian(DWHActor* i);
void premapImp(DWHActor* i);
void premapJudy(DWHActor* i);
void premapKatie(DWHActor* i);
void premapKobold(DWHActor* i);
void premapKurt(DWHActor* i);
void premapMinotaur(DWHActor* i);
void premapNewGuy(DWHActor* i);
void premapRat(DWHActor* i);
void premapSkeleton(DWHActor* i);
void premapSkully(DWHActor* i);
void premapSpider(DWHActor* i);
void premapWillow(DWHActor* i);
void deaddude(short sn);


inline int findplayer() {
	return 0; // no multiplayer support, apparently...
}

END_WH_NS
