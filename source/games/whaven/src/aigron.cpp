#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexplgron(PLAYER& plr, DWHActor* i);
static void throwhalberd(int s);


static void chasegron(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (spr.picnum == GRONSW) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			if (checkdist(plr, actor)) {
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
				}
				else
					SetNewStatus(actor, ATTACK);
			}
			else if (krand() % 63 > 60) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			else {
				auto moveStat = aimove(actor);
				if (moveStat.type == kHitFloor)
				{
					spr.ang = (short)((spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
					return;
				}

				if (moveStat.type == kHitSprite) {
					if (moveStat.actor != plr.actor()) {
						short daang = (short)((spr.ang - 256) & 2047);
						spr.ang = daang;
						if (plr.shadowtime > 0) {
							spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
							SetNewStatus(actor, FLEE);
						}
						else
							SetNewStatus(actor, SKIRMISH);
					}
					else {
						spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						SetNewStatus(actor, SKIRMISH);
					}
				}
			}
		}
		else {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
	}
	else {
		if (krand() % 63 == 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
				spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum))// && invisibletime < 0)
				SetNewStatus(actor, ATTACK);
		}
		else {
			checksight(plr, actor);
			if (!checkdist(plr, actor)) {
				if (aimove(actor).type == kHitFloor)
				{
					spr.ang = (short)((spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
					return;
				}
			}
			else {
				if (krand() % 8 == 0) // NEW
					SetNewStatus(actor, ATTACK); // NEW
				else { // NEW
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
					SetNewStatus(actor, FLEE); // NEW
				}
			}
		}
	}

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florHit, false);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	SetActorPos(actor, &spr.pos);

	if (zr_florHit.type == kHitSector && (spr.sector()->floorpicnum == LAVA
		|| spr.sector()->floorpicnum == LAVA1 || spr.sector()->floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	checkexplgron(plr, actor);
}
	
static void resurectgron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		switch (krand() % 3) {
		case 0:
			spr.picnum = GRONHAL;
			spr.hitag = (short)adjusthp(120);
			spr.extra = 3;
			break;
		case 1:
			spr.picnum = GRONSW;
			spr.hitag = (short)adjusthp(120);
			spr.extra = 0;
			break;
		case 2:
			spr.picnum = GRONMU;
			spr.hitag = (short)adjusthp(120);
			spr.extra = 2;
			break;
		}
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void skirmishgron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag < 0)
		SetNewStatus(actor, FACE);
	short osectnum = spr.sectnum;
	auto moveStat = aimove(actor);
	if (moveStat.type != kHitFloor && moveStat.type != kHitNone) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		SetNewStatus(actor, FACE);
	}
	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	processfluid(actor, zr_florHit, false);

	SetActorPos(actor, &spr.pos);

	if (checksector6(actor))
		return;

	checkexplgron(plr, actor);
}
	
static void searchgron(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	if (!checksector6(actor))
		checkexplgron(plr, actor);
}
	
static void nukedgron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	if (isWh2()) {
		chunksofmeat(plr, actor,spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
		trailingsmoke(actor,false);
		SetNewStatus(actor, DIE);
		return;
	}

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == GRONCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}
	
static void frozengron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		if (spr.picnum == GRONHALDIE)
			spr.picnum = GRONHAL;
		else if (spr.picnum == GRONSWDIE)
			spr.picnum = GRONSW;
		else if (spr.picnum == GRONMUDIE)
			spr.picnum = GRONMU;
		SetNewStatus(actor, FACE);
	}
}
			
static void paingron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (spr.picnum == GRONHALPAIN)
			spr.picnum = GRONHAL;
		else if (spr.picnum == GRONSWPAIN)
			spr.picnum = GRONSW;
		else if (spr.picnum == GRONMUPAIN)
			spr.picnum = GRONMU;

		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);

	checkexplgron(plr, actor);
}
	
static void facegron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = (short)(getangle(plr.x - spr.x, plr.y - spr.y) & 2047);

		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
		else {
			spr.owner = plr.spritenum;
			SetNewStatus(actor, CHASE);
		}
	}
	else { // get off the wall
		if (spr.owner == plr.spritenum) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang) & 2047);
			SetNewStatus(actor, FINDME);
		}
		else if (cansee) SetNewStatus(actor, FLEE);
	}

	if (checkdist(plr, actor))
		SetNewStatus(actor, ATTACK);

	checkexplgron(plr, actor);
}
	
static void attackgron(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	if (spr.picnum == GRONSWATTACK) {
		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
		spr.z = zr_florz;

		switch (checkfluid(actor, zr_florHit)) {
		case TYPELAVA:
			spr.hitag--;
			if (spr.hitag < 0)
				SetNewStatus(actor, DIE);
		case TYPEWATER:
			spr.z += tileHeight(spr.picnum) << 5;
			break;
		}

		SetActorPos(actor, &spr.pos);

		if (spr.lotag == 31) {
			if (checksight(plr, actor))
				if (checkdist(plr, actor)) {
					spr.ang = (short)checksight_ang;
					attack(plr, i);
				}
		}
		else if (spr.lotag < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
		spr.lotag -= TICSPERFRAME;
	}
	else {
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
				spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum))
				SetNewStatus(actor, CAST);
			else
				SetNewStatus(actor, CHASE);
		}
		else
			spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	}

	checksector6(actor);
}
	
static void fleegron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	auto moveStat = aimove(actor);
	if (moveStat.type != kHitFloor && moveStat.type != kHitNone) {
		if (moveStat.type == kHitWall) {
			int nWall = moveStat.index;
			int nx = -(wall[wall[nWall].point2].y - wall[nWall].y) >> 4;
			int ny = (wall[wall[nWall].point2].x - wall[nWall].x) >> 4;
			spr.ang = getangle(nx, ny);
		}
		else {
			spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
			SetNewStatus(actor, FACE);
		}
	}
	if (spr.lotag < 0)
		SetNewStatus(actor, FACE);

	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florHit, false);

	SetActorPos(actor, &spr.pos);

	checkexplgron(plr, actor);
}
	
static void castgron(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (spr.picnum == GRONHALATTACK) {
			spr.extra--;
			spritesound(S_THROWPIKE, actor);
			throwhalberd(i);
			SetNewStatus(actor, CHASE);
		}
		else if (spr.picnum == GRONMUATTACK) {
			spr.extra--;
			spritesound(S_SPELL2, actor);
			castspell(plr, actor);
			SetNewStatus(actor, CHASE);
		}
	}

	checksector6(actor);
}
	
static void diegron(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.picnum == GRONSWDIE || spr.picnum == GRONHALDIE || spr.picnum == GRONMUDIE)
	{
		if (spr.lotag < 0) {
			spr.picnum = GRONDIE;
			spr.lotag = 20;
		}
		else
			return;
	}

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == GRONDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

static void checkexplgron(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	WHSectIterator it(spr.sectnum);
	while (auto sectactor = it.Next())
	{
		SPRITE& spri = sectactor->s();
		int j = sectactor->GetSpriteIndex();

		int dx = abs(spr.x - spri.x); // x distance to sprite
		int dy = abs(spr.y - spri.y); // y distance to sprite
		int dz = abs((spr.z >> 8) - (spri.z >> 8)); // z distance to sprite
		int dh = tileHeight(spri.picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (spri.picnum == EXPLO2
				|| spri.picnum == MONSTERBALL) {
				spr.hitag -= TICSPERFRAME << 2;
				if (spr.hitag < 0) {
					SetNewStatus(actor, DIE);
				}
			}
		}
	}
}

static void throwhalberd(int s) {
	int j = insertsprite(sprite[s].sectnum, JAVLIN);
	auto spawnedactor = &whActors[j];

	if (j == -1)
		return;
	SPRITE& spr = spawnedactor->s();
	spr.x = sprite[s].x;
	spr.y = sprite[s].y;
	spr.z = sprite[s].z - (40 << 8);

	spr.cstat = 17;

	spr.picnum = THROWHALBERD;
	spr.detail = THROWHALBERDTYPE;
	spr.ang = (short)(((sprite[s].ang + 2048) - 512) & 2047);
	spr.xrepeat = 8;
	spr.yrepeat = 16;
	spr.clipdist = 32;

	spr.extra = sprite[s].ang;
	spr.shade = -15;
	spr.xvel = (short)((krand() & 256) - 128);
	spr.yvel = (short)((krand() & 256) - 128);
	spr.zvel = (short)((krand() & 256) - 128);
	spr.owner = (short)s;
	spr.lotag = 0;
	spr.hitag = 0;
	spr.pal = 0;

	spr.cstat = 0;
	int daz = (((spr.zvel) * TICSPERFRAME) >> 3);
	movesprite(spawnedactor, (bcos(spr.extra) * TICSPERFRAME) << 7,
		(bsin(spr.extra) * TICSPERFRAME) << 7, daz, 4 << 8, 4 << 8, 1);
	spr.cstat = 21;
	spr.backuploc();
}

void createGronAI() {
	auto& e = enemy[GRONTYPE];
	e.info.Init(isWh2() ? 35 : 30, isWh2() ? 35 : 30, -1, 120, 0, 64, false, 300, 0);
	e.info.getAttackDist = [](EnemyInfo& e, SPRITE& spr)
	{
		int out = e.attackdist;
		int pic = spr.picnum;

		if (pic == GRONHAL || pic == GRONHALATTACK)
			out = 1024 + 512;
		else if (pic == GRONMU || pic == GRONMUATTACK)
			out = 2048;
		else  if (pic == GRONSW || pic == GRONSWATTACK)
			out = 1024 + 256;

		return out;
	};

	e.info.getHealth = [](EnemyInfo& e, SPRITE& spr)
	{
		if (isWh2()) {
			if (spr.picnum == GRONHAL)
				return adjusthp(65);
			if (spr.picnum == GRONMU)
				return adjusthp(70);
		}
		return adjusthp(e.health);
	};
	e.chase = chasegron;
	e.resurect = resurectgron;
	e.skirmish = skirmishgron;
	e.search = searchgron;
	e.nuked = nukedgron;
	e.frozen = frozengron;
	e.pain = paingron;
	e.face = facegron;
	e.attack = attackgron;
	e.flee = fleegron;
	e.cast = castgron;
	e.die = diegron;
}


void premapGron(short i) {
	SPRITE& spr = sprite[i];

	if (spr.picnum == GRONSW && spr.pal == 10)
		deletesprite(i);

	spr.detail = GRONTYPE;
	enemy[GRONTYPE].info.set(spr);
	changespritestat(i, FACE);

	if (spr.picnum == GRONHAL)
		spr.extra = 4;
	else if (spr.picnum == GRONSW)
		spr.extra = 0;
	else if (spr.picnum == GRONMU)
		spr.extra = 2;
}

END_WH_NS
