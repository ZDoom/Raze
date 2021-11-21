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
		this->sizx =  sizx;
		this->sizy =  sizy;
		this->attackdist = dist;
		this->attackheight = height;
		this->attackdamage = damage;
		this->clipdist = clipdist;
		this->fly = fly;
		this->health =  health;
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

		spr.cstat =  (0x101 | tflag);
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
Collision aifly(DWHActor* actor);
void aisearch(PLAYER& plr, DWHActor* actor, boolean fly);
boolean checksector6(DWHActor* actor);
int checkfluid(DWHActor* actor, Collision& florhit);
void processfluid(DWHActor* actor, Collision& florHit, boolean fly);
void castspell(PLAYER& plr, DWHActor* actor);
void skullycastspell(PLAYER& plr, DWHActor*);
void attack(PLAYER& plr, DWHActor* actor);
int checkmove(DWHActor* actor, int dax, int day);
boolean checkdist(PLAYER& plr, DWHActor* actor);
boolean checkdist(DWHActor* actor, int x, int y, int z);
extern int checksight_ang;
boolean checksight(PLAYER& plr, DWHActor* actor);
void monsterweapon(DWHActor*);
PLAYER* aiGetPlayerTarget(DWHActor*);
boolean actoruse(DWHActor* i);

void initAI();
void aiInit();

void premapDemon(DWHActor* actor);
void premapDevil(DWHActor* actor);
void premapDragon(DWHActor* actor);
void premapFatwitch(DWHActor* actor);
void premapFish(DWHActor* actor);
void premapFred(DWHActor* actor);
void premapGoblin(DWHActor* actor);
void premapGonzo(DWHActor* actor);
void premapGron(DWHActor* actor);
void premapGuardian(DWHActor* actor);
void premapImp(DWHActor* actor);
void premapJudy(DWHActor* actor);
void premapKatie(DWHActor* actor);
void premapKobold(DWHActor* actor);
void premapKurt(DWHActor* actor);
void premapMinotaur(DWHActor* actor);
void premapNewGuy(DWHActor* actor);
void premapRat(DWHActor* actor);
void premapSkeleton(DWHActor* actor);
void premapSkully(DWHActor* actor);
void premapSpider(DWHActor* actor);
void premapWillow(DWHActor* actor);
void deaddude(DWHActor* sn);


inline int findplayer() {
	return 0; // no multiplayer support, apparently...
}

END_WH_NS
