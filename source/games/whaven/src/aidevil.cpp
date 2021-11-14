#include "ns.h"
#include "wh.h"

BEGIN_WH_NS


static void chasedevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum) && plr.invisibletime < 0)
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
			if (plr.invisibletime < 0) {
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
}

static void diedevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == DEVILDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

static void paindevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = DEVIL;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);
}

static void facedevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
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
}

static void fleedevil(PLAYER& plr, DWHActor* actor)
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
}

static void attackdevil(PLAYER& plr, DWHActor* actor)
{
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

	spr.extra -= TICSPERFRAME;
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

static void resurectdevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = DEVIL;
		spr.hitag = (short)adjusthp(60);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void searchdevil(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	checksector6(actor);
}

static void frozendevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = DEVIL;
		SetNewStatus(actor, FACE);
	}
}

static void nukeddevil(PLAYER& plr, DWHActor* actor)
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
		if (spr.picnum == DEVILCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}

static void castdevil(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == DEVILATTACK + 2) {
		spr.picnum = DEVIL;
		spritesound(S_FIREBALL, actor);
		castspell(plr, actor);
		SetNewStatus(actor, CHASE);
	}
	checksector6(actor);
}


void createDevilAI() {
	auto &e = enemy[DEVILTYPE];
	e.info.Init(isWh2() ? 50 : 36, isWh2() ? 50 : 36, 2048, 120, 0, 64, false, 50, 0);
	e.chase = chasedevil;
	e.die = diedevil;
	e.pain = paindevil;
	e.face = facedevil;
	e.flee = fleedevil;
	e.attack = attackdevil;
	e.resurect = resurectdevil;
	e.search = searchdevil;
	e.frozen = frozendevil;
	e.nuked = nukeddevil;
	e.cast = castdevil;
}

void premapDevil(DWHActor* actor) {
	SPRITE& spr = actor->s();
	spr.detail = DEVILTYPE;
	ChangeActorStat(actor, FACE);
	enemy[DEVILTYPE].info.set(spr);
	if (spr.pal == 2)
		spr.hitag = adjusthp(60);
}

END_WH_NS
