#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexplskeleton(PLAYER& plr, DWHActor* i);

static void chaseskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
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

	checkexplskeleton(plr, actor);
}

static void dieskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == SKELETONDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

static void faceskeleton(PLAYER& plr, DWHActor* actor)
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

	checkexplskeleton(plr, actor);
}

static void fleeskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.picnum = SKELETON;

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

	checkexplskeleton(plr, actor);
}

static void standskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {

			if (spr.picnum == HANGMAN) {
				SetNewStatus(actor, CHILL);
				spritesound(S_SKELSEE, actor);
			}
			else {
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
					SetNewStatus(actor, FLEE);
				}
				else
					SetNewStatus(actor, CHASE);
			}
		}
	}

	checksector6(actor);
}
	
static void attackskeleton(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(actor, zr_florHit)) {
	case TYPELAVA:
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	SetActorPos(actor, &spr.pos);

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	if (spr.lotag == 16) {
		if (checksight(plr, actor))
			if (checkdist(plr, actor)) {
				spr.ang = (short)checksight_ang;
				attack(plr, actor);
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

	checksector6(actor);
}
	
static void resurectskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = SKELETON;
		spr.hitag = (short)adjusthp(10);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void searchskeleton(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	if (!checksector6(actor))
		checkexplskeleton(plr, actor);
}
	
static void frozenskeleton(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = SKELETON;
		SetNewStatus(actor, FACE);
	}
}
	
static void nukedskeleton(PLAYER& plr, DWHActor* actor)
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
		if (spr.picnum == SKELETONCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}
	
static void skirmishskeleton(PLAYER& plr, DWHActor* actor)
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

	checkexplskeleton(plr, actor);
}

void skeletonChill(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 18;
		if (spr.picnum == HANGMAN + 10) {
			spr.picnum = SKELETON;
			SetNewStatus(actor, FACE);
		}
	}
}

static void checkexplskeleton(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	WHSectIterator it(spr.sectnum);
	while (auto sectactor = it.Next())
	{
		SPRITE& tspr = sectactor->s();

		int dx = abs(spr.x - tspr.x); // x distance to sprite
		int dy = abs(spr.y - tspr.y); // y distance to sprite
		int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
		int dh = tileHeight(tspr.picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (tspr.picnum == EXPLO2
				|| tspr.picnum == SMOKEFX
				|| tspr.picnum == MONSTERBALL) {
				spr.hitag -= TICSPERFRAME << 2;
				if (spr.hitag < 0) {
					SetNewStatus(actor, DIE);
				}
			}
		}
	}
}

void createSkeletonAI() {
	auto& e = enemy[SKELETONTYPE];
	e.info.Init(isWh2() ? 35 : 24, isWh2() ? 35 : 24, 1024, 120, 0, 64, false, isWh2() ? 25 : 30, 0);
	e.chase = chaseskeleton;
	e.die = dieskeleton;
	e.face = faceskeleton;
	e.flee = fleeskeleton;
	e.stand = standskeleton;
	e.attack = attackskeleton;
	e.resurect = resurectskeleton;
	e.search = searchskeleton;
	e.frozen = frozenskeleton;
	e.nuked = nukedskeleton;
	e.skirmish = skirmishskeleton;
}
	

void premapSkeleton(DWHActor* actor) {
	SPRITE& spr = actor->s();

	spr.detail = SKELETONTYPE;
	enemy[SKELETONTYPE].info.set(spr);

	if (spr.picnum == HANGMAN) {
		spr.xrepeat = 28;
		spr.yrepeat = 28;
		ChangeActorStat(actor, STAND);
		return;
	}

	ChangeActorStat(actor, FACE);
}

END_WH_NS
