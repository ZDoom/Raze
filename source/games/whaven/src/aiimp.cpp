#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexplimp(PLAYER& plr, DWHActor* i);

static void chaseimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && plr.invisibletime < 0) {
		if (checkdist(plr, i)) {
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
			int movestat = aimove(i);
			if ((movestat & kHitTypeMask) == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
				return;
			}

			if ((movestat & kHitTypeMask) == kHitSprite) {
				if ((movestat & kHitIndexMask) != plr.spritenum) {
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

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, false);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	checkexplimp(plr, actor);
}
		
static void frozenimp(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = IMP;
		SetNewStatus(actor, FACE);
	}
}
	
static void painimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = IMP;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}
	
static void dieimp(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == IMPDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}
	
static void nukedimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(actor,false);
	SetNewStatus(actor, DIE);
}
	
static void resurectimp(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = IMP;
		spr.hitag = (short)adjusthp(20);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void faceimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
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

	if (checkdist(plr, i))
		SetNewStatus(actor, ATTACK);

	checkexplimp(plr, actor);
}

static void fleeimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	int movestat = aimove(i);

	if ((movestat & kHitTypeMask) != kHitFloor && movestat != 0) {
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
		warpsprite(i);

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, false);

	setsprite(i, spr.x, spr.y, spr.z);

	checkexplimp(plr, actor);
}
	
static void attackimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	setsprite(i, spr.x, spr.y, spr.z);
	if (spr.lotag == 32) { //original 64
		if (checksight(plr, i))
			if (checkdist(plr, i)) {
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

	checksector6(i);
}
	
static void searchimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();

	aisearch(plr, i, false);
	if (!checksector6(i))
		checkexplimp(plr, actor);
}
		
static void skirmishimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag < 0)
		SetNewStatus(actor, FACE);
	short osectnum = spr.sectnum;
	int movestat = aimove(i);
	if ((movestat & kHitTypeMask) != kHitFloor && movestat != 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		SetNewStatus(actor, FACE);
	}
	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	processfluid(i, zr_florhit, false);

	setsprite(i, spr.x, spr.y, spr.z);

	if (checksector6(i))
		return;

	checkexplimp(plr, actor);
}

static void checkexplimp(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	WHSectIterator it(spr.sectnum);
	while (auto sect = it.Next())
	{
		SPRITE& tspr = sect->s();
		int j = sect->GetSpriteIndex();

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

void createImpAI() {
	auto& e = enemy[IMPTYPE];
	e.info.Init(25, 25, 1024, 120, 0, 64, false, 20, 0);
	e.chase = chaseimp;
	e.frozen = frozenimp;
	e.pain = painimp;
	e.die = dieimp;
	e.nuked = nukedimp;
	e.resurect = resurectimp;
	e.face = faceimp;
	e.flee = fleeimp;
	e.attack = attackimp;
	e.search = searchimp;
	e.skirmish = skirmishimp;
}

	
void premapImp(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = IMPTYPE;
	enemy[IMPTYPE].info.set(spr);
	changespritestat(i, FACE);
	spr.shade = -4;
}

END_WH_NS
