#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void chaseguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	if (plr.z < spr.z)
		spr.z -= TICSPERFRAME << 8;
	if (plr.z > spr.z)
		spr.z += TICSPERFRAME << 8;

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum) && plr.invisibletime < 0)
			SetNewStatus(actor, ATTACK);
		return;
	}
	else {
		int dax = (bcos(spr.ang) * TICSPERFRAME) << 3;
		int day = (bsin(spr.ang) * TICSPERFRAME) << 3;
		checksight(plr, actor);

		if (PlayClock % 100 > 70)
			trailingsmoke(actor,true);

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

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, true);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);

	if (!isValidSector(spr.sectnum))
		return;

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}
}
	
static void nukedguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	if (isWh2()) {
		chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
		trailingsmoke(actor,false);
		SetNewStatus(actor, DIE);
		return;
	}

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == GUARDIANCHAR + 4) {
			trailingsmoke(actor,false);
			deletesprite(i);
		}
	}
}
	
static void attackguardian(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	if (plr.z < spr.z) {
		spr.z -= TICSPERFRAME << 8;
	}
	if (plr.z > spr.z) {
		spr.z += TICSPERFRAME << 8;
	}

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
	
static void faceguardian(PLAYER& plr, DWHActor* actor)
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
}
	
static void searchguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();

	aisearch(plr, i, true);
	checksector6(i);
}
	
static void fleeguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	if (PlayClock % 100 > 70)
		trailingsmoke(actor,true);

	int movestat = aifly(i);

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

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, true);

	setsprite(i, spr.x, spr.y, spr.z);
}
	
static void painguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = GUARDIAN;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	//				aifly(i);
	//				setsprite(i, spr.x, spr.y, spr.z);
}
	
static void castguardian(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (plr.z < spr.z) {
		spr.z -= TICSPERFRAME << 8;
	}
	if (plr.z > spr.z) {
		spr.z += TICSPERFRAME << 8;
	}

	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == GUARDIANATTACK + 6) {
		spr.picnum = GUARDIAN;
		spritesound(S_FIREBALL, &sprite[i]);
		castspell(plr, i);
		SetNewStatus(actor, CHASE);
	}
	checksector6(i);
}


void createGuardianAI() {
	auto& e = enemy[GUARDIANTYPE];
	e.info.Init(isWh2() ? 35 : 32, isWh2() ? 35 : 32, 4096, 120, 0, 64, true, isWh2() ? 100 : 200, 0);
	e.chase = chaseguardian;
	e.nuked = nukedguardian;
	e.attack = attackguardian;
	e.face = faceguardian;
	e.search = searchguardian;
	e.flee = fleeguardian;
	e.pain = painguardian;
	e.cast = castguardian;
}

void premapGuardian(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = GUARDIANTYPE;
	enemy[GUARDIANTYPE].info.set(spr);
	changespritestat(i, FACE);
}

END_WH_NS
