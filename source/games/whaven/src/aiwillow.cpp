#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void willowDrain(PLAYER& plr, DWHActor* i);

static void chasewillow(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum) && plr.invisibletime < 0)
			SetNewStatus(actor, ATTACK);
		return;
	}
	else {
		//spr.z = sector[spr.sectnum].floorz - (32 << 8);
		int dax = (bcos(spr.ang) * TICSPERFRAME) << 3;
		int day = (bsin(spr.ang) * TICSPERFRAME) << 3;
		checksight(plr, actor);

		if (!checkdist(plr, i)) {
			checkmove(actor, dax, day);
		}
		else {
			if (krand() % 8 == 0) // NEW
				SetNewStatus(actor, ATTACK); // NEW
			else { // NEW
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, CHASE); // NEW
			}
		}
	}

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(actor);

	if (spr.z > zr_florz)
		spr.z = zr_florz;
	if (spr.z < zr_ceilz - (32 << 8))
		spr.z = zr_ceilz - (32 << 8);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florhit, true);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	SetActorPos(actor, &spr.pos);

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}
}
	
static void attackwillow(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (plr.z < spr.z)
		spr.z -= TICSPERFRAME << 8;

	if (plr.z > spr.z)
		spr.z += TICSPERFRAME << 8;

	if (spr.lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum))
			if (checkdist(plr, i)) {
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
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && plr.invisibletime < 0) {
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
		else SetNewStatus(actor, FLEE);
	}

	if (checkdist(plr, i))
		SetNewStatus(actor, ATTACK);
}
	
static void searchwillow(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	aisearch(plr, actor, true);
	checksector6(actor);
}
	
static void fleewillow(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	int movestat = aifly(actor);

	if (movestat != 0) {
		if ((movestat & kHitTypeMask) == kHitWall) {
			int nWall = movestat & kHitIndexMask;
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

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florhit, true);

	SetActorPos(actor, &spr.pos);
}
	
static void diewillow(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
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
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	if (isWh2()) {
		chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
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
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spritesound(S_FIREBALL, &spr);
		int oldz = spr.z;
		spr.z += 6144;
		castspell(plr, i);
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
	

void premapWillow(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = WILLOWTYPE;
	enemy[WILLOWTYPE].info.set(spr);
	spr.cstat |= 128;
	spr.z -= tileHeight(WILLOW) << 8;
	changespritestat(i, FACE);
}

END_WH_NS
