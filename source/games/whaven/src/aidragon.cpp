#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static int checksight_x, checksight_y = 0;

static void dragonAttack2(PLAYER& plr, DWHActor* i);
static void firebreath(PLAYER& plr, DWHActor*, int a, int b, int c);

static void chasedragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	//				int speed = 10;
	if ((krand() % 16) == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0)
			if (plr.z < spr.z)
				SetNewStatus(actor, ATTACK2);
			else
				SetNewStatus(actor, ATTACK);
		return;
	}
	else {
		int dax = (bcos(spr.ang) * TICSPERFRAME) << 3;
		int day = (bsin(spr.ang) * TICSPERFRAME) << 3;
		checksight(plr, actor);
		if (!checkdist(plr, actor)) {
			checkmove(actor, dax, day);
		}
		else {
			if (plr.invisibletime < 0) {
				if (krand() % 8 == 0) { // NEW
					if (plr.z < spr.z)
						SetNewStatus(actor, ATTACK2);
					else
						SetNewStatus(actor, ATTACK);
				}
				else { // NEW
					SetNewStatus(actor, FACE); // NEW
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

static void fleedragon(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
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

static void diedragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();
	
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;
		if (spr.picnum == DRAGONDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

static void castdragon(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();
	
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	switch (spr.picnum) {
	case DRAGONATTACK + 17:
	case DRAGONATTACK + 4:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, actor);
		else
			spritesound(S_FIREBALL, actor);

		firebreath(plr, actor, 1, 2, LOW);
		break;
	case DRAGONATTACK + 18:
	case DRAGONATTACK + 5:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, actor);
		else
			spritesound(S_FIREBALL, actor);

		firebreath(plr, actor, 2, 1, LOW);
		break;
	case DRAGONATTACK + 19:
	case DRAGONATTACK + 6:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, actor);
		else
			spritesound(S_FIREBALL, actor);

		firebreath(plr, actor, 4, 0, LOW);
		break;
	case DRAGONATTACK + 20:
	case DRAGONATTACK + 7:
		firebreath(plr, actor, 2, -1, LOW);
		break;
	case DRAGONATTACK + 21:
	case DRAGONATTACK + 8:
		firebreath(plr, actor, 1, -2, LOW);
		break;

	case DRAGONATTACK2 + 2:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, actor);
		else
			spritesound(S_FIREBALL, actor);

		firebreath(plr, actor, 1, -1, HIGH);
		break;
	case DRAGONATTACK2 + 3:
		firebreath(plr, actor, 2, 0, HIGH);
		break;

	case DRAGONATTACK2 + 5:
		spr.picnum = DRAGON;
		SetNewStatus(actor, CHASE);
		break;
	case DRAGONATTACK + 22:
		spr.picnum = DRAGONATTACK;
		SetNewStatus(actor, CHASE);
		break;
	}

	checksector6(actor);
}

static void attackdragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum))
			SetNewStatus(actor, CAST);
		else
			SetNewStatus(actor, CHASE);
	}
	else
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
}

static void resurectdragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = DRAGON;
		spr.hitag = (short)adjusthp(900);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void searchdragon(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, true);
	checksector6(actor);
}

static void frozendragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = DRAGON;
		SetNewStatus(actor, FACE);
	}
}

static void nukeddragon(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == DRAGONCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}

static void paindragon(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);
}

static void facedragon(PLAYER& plr, DWHActor* actor)
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


void dragonProcess(PLAYER& plr)
{
	WHStatIterator it(ATTACK2);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		switch (spr.detail) {
		case DRAGON:
			dragonAttack2(plr, actor);
			break;
		}
	}
}

static void dragonAttack2(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0)
			SetNewStatus(actor, CAST);
		else
			SetNewStatus(actor, CHASE);
		return;
	}
	else
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);

	checksector6(actor);
}

static void firebreath(PLAYER& plr, DWHActor* actor, int a, int b, int c) 
{
	auto& spr = actor->s();
	for (int k = 0; k <= a; k++) {
		auto spawnedactor = InsertActor(spr.sectnum, MISSILE);
		auto& spawned = spawnedactor->s();

		spawned.x = spr.x;
		spawned.y = spr.y;
		if (c == LOW)
			spawned.z = spr.sector()->floorz - (32 << 8);
		else
			spawned.z = spr.sector()->floorz - (tileHeight(spr.picnum) << 7);
		spawned.cstat = 0;
		spawned.picnum = MONSTERBALL;
		spawned.shade = -15;
		spawned.xrepeat = 128;
		spawned.yrepeat = 128;
		spawned.ang = (short)((((getangle(plr.x - spawned.x, plr.y - spawned.y)
			+ (krand() & 15) - 8) + 2048) + ((b * 22) + (k * 10))) & 2047);
		spawned.xvel = bcos(spawned.ang, -6);
		spawned.yvel = bsin(spawned.ang, -6);
		int discrim = ksqrt(
			(plr.x - spawned.x) * (plr.x - spawned.x) + (plr.y - spawned.y) * (plr.y - spawned.y));
		if (discrim == 0)
			discrim = 1;
		if (c == HIGH)
			spawned.zvel = (short)(((plr.z + (32 << 8) - spawned.z) << 7) / discrim);
		else
			spawned.zvel = (short)((((plr.z + (8 << 8)) - spawned.z) << 7) / discrim);// NEW

		spawned.owner = actor->GetSpriteIndex();
		spawned.clipdist = 16;
		spawned.lotag = 512;
		spawned.hitag = 0;
		spawned.backuploc();
	}
}
	
void createDragonAI() {
	auto& e = enemy[DRAGONTYPE];
	e.info.Init(54, 54, 512, 120, 0, 128, false, 900, 0);
	e.chase = chasedragon;
	e.flee = fleedragon;
	e.die = diedragon;
	e.cast = castdragon;
	e.attack = attackdragon;
	e.resurect = resurectdragon;
	e.search = searchdragon;
	e.frozen = frozendragon;
	e.nuked = nukeddragon;
	e.pain = paindragon;
	e.face = facedragon;
}

void premapDragon(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = DRAGONTYPE;
	changespritestat(i, FACE);
	enemy[DRAGONTYPE].info.set(spr);
}

END_WH_NS
