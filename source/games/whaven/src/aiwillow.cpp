#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void willowDrain(PLAYER& plr, DWHActor* actor);

static void chasewillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	auto osectnum = spr.sectnum;
	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum) && plr.invisibletime < 0)
			SetNewStatus(actor, ATTACK);
		return;
	}
	else {
		//spr.z = spr.sector()->floorz - (32 << 8);
		int dax = (bcos(spr.ang) * TICSPERFRAME) << 3;
		int day = (bsin(spr.ang) * TICSPERFRAME) << 3;
		checksight(plr, actor);

		if (!checkdist(plr, actor)) {
			checkmove(actor, dax, day);
		}
		else {
			if (krand() % 8 == 0) // NEW
				SetNewStatus(actor, ATTACK); // NEW
			else { // NEW
				spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, CHASE); // NEW
			}
		}
	}

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	if (spr.z > zr_florz)
		spr.z = zr_florz;
	if (spr.z < zr_ceilz - (32 << 8))
		spr.z = zr_ceilz - (32 << 8);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florHit, true);

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
}
	
static void attackwillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (plr.z < spr.z)
		spr.z -= TICSPERFRAME << 8;

	if (plr.z > spr.z)
		spr.z += TICSPERFRAME << 8;

	if (spr.lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum))
			if (checkdist(plr, actor)) {
				if (plr.shockme < 0)
					if ((krand() & 1) != 0) {
						plr.shockme = 120;
						if (!isWh2()) {
							plr.lvl--;
							switch (plr.lvl) {
							case 1:
								plr.score = 0;
								plr.maxhealth = 100;
								break;
							case 2:
								plr.score = 2350;
								plr.maxhealth = 120;
								break;
							case 3:
								plr.score = 4550;
								plr.maxhealth = 140;
								break;
							case 4:
								plr.score = 9300;
								plr.maxhealth = 160;
								break;
							case 5:
								plr.score = 18400;
								plr.maxhealth = 180;
								break;
							case 6:
								plr.score = 36700;
								plr.maxhealth = 200;
								break;
							case 7:
								plr.score = 75400;
								plr.maxhealth = 200;
								break;
							}
							if (plr.lvl < 1) {
								plr.lvl = 1;
								plr.health = -1;
							}
							showmessage("Level Drained", 360);
						}
						else
							showmessage("Shocked", 360);

					}
			}
			else
				SetNewStatus(actor, DRAIN);
		else
			SetNewStatus(actor, CHASE);
	}

	int floorz = getflorzofslope(spr.sectnum, spr.x, spr.y) - (16 << 8);
	if (spr.z > floorz)
		spr.z = floorz;
}
	
static void facewillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && plr.invisibletime < 0) {
		spr.ang = (getangle(plr.x - spr.x, plr.y - spr.y) & 2047);

		if (plr.shadowtime > 0) {
			spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
		else {
			actor->SetOwner(plr.actor());
			SetNewStatus(actor, CHASE);
		}
	}
	else { // get off the wall
		if (actor->GetOwner() == plr.actor()) {
			spr.ang = (((krand() & 512 - 256) + spr.ang) & 2047);
			SetNewStatus(actor, FINDME);
		}
		else SetNewStatus(actor, FLEE);
	}

	if (checkdist(plr, actor))
		SetNewStatus(actor, ATTACK);
}
	
static void searchwillow(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, true);
	checksector6(actor);
}
	
static void fleewillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	auto osectnum = spr.sectnum;

	auto moveStat = aifly(actor);

	if (moveStat.type != kHitNone) {
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

	processfluid(actor, zr_florHit, true);

	SetActorPos(actor, &spr.pos);
}
	
static void diewillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;
		if (spr.picnum == WILLOWEXPLO || spr.picnum == WILLOWEXPLO + 1
			|| spr.picnum == WILLOWEXPLO + 2)
			spr.xrepeat = spr.yrepeat <<= 1;

		if (spr.picnum == WILLOWEXPLO + 2) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}
		
static void nukedwillow(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	if (isWh2()) {
		chunksofmeat(plr, actor,spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
		trailingsmoke(actor,false);
		SetNewStatus(actor, DIE);
	}
}

void willowProcess(PLAYER& plr)
{
	WHStatIterator it(DRAIN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		switch (spr.detail) {
		case WILLOWTYPE:
			willowDrain(plr, actor);
			break;
		}
	}
}
	
static void willowDrain(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spritesound(S_FIREBALL, actor);
		int oldz = spr.z;
		spr.z += 6144;
		castspell(plr, actor);
		spr.z = oldz;
		SetNewStatus(actor, CHASE);
	}
}

void createWillowAI() {
	auto& e = enemy[WILLOWTYPE];
	e.info.Init(32, 32, 512, 120, 0, 64, true, isWh2() ? 5 : 400, 0);
	e.chase = chasewillow;
	e.attack = attackwillow;
	e.face = facewillow;
	e.search = searchwillow;
	e.flee = fleewillow;
	e.die = diewillow;
	e.nuked = nukedwillow;
}
	

void premapWillow(DWHActor* actor) {
	SPRITE& spr = actor->s();

	spr.detail = WILLOWTYPE;
	enemy[WILLOWTYPE].info.set(spr);
	spr.cstat |= 128;
	spr.z -= tileHeight(WILLOW) << 8;
	ChangeActorStat(actor, FACE);
}

END_WH_NS
