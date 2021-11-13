#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void throwspank(PLAYER& plr, DWHActor* i);

static void chasefatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum))// && invisibletime < 0)
			SetNewStatus(actor, ATTACK);
	}
	else {
		checksight(plr, actor);
		if (!checkdist(i, plr.x, plr.y, plr.z)) {
			int movestat = aimove(i);
			if ((movestat & kHitTypeMask) == kHitFloor)
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

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(i, zr_florhit, false);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	SetActorPos(actor, &spr.pos);
}
	
static void resurectfatwitch(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = FATWITCH;
		spr.hitag = (short)adjusthp(90);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void searchfatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();

	aisearch(plr, i, false);
	checksector6(actor);
}
	
static void nukedfatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == FATWITCHCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}
	
static void painfatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = FATWITCH;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	SetActorPos(actor, &spr.pos);
}
	
static void facefatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum);

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

	if (checkdist(i, plr.x, plr.y, plr.z))
		SetNewStatus(actor, ATTACK);
}
	
static void attackfatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	SetActorPos(actor, &spr.pos);

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
	
static void fleefatwitch(PLAYER& plr, DWHActor* actor)
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
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(i, zr_florhit, false);

	SetActorPos(actor, &spr.pos);
}
	
static void castfatwitch(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == FATWITCHATTACK + 3) {
		spr.picnum = FATWITCH;
		throwspank(plr, actor);
		SetNewStatus(actor, CHASE);
	}
	checksector6(actor);
}
	
static void diefatwitch(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == FATWITCHDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}


static void throwspank(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	int j = insertsprite(spr.sectnum, MISSILE);
	if (j == -1)
		return;

	spritesound(S_WITCHTHROW, &spr);

	sprite[j].x = spr.x;
	sprite[j].y = spr.y;
	sprite[j].z = sector[spr.sectnum].floorz - ((tileHeight(spr.picnum) >> 1) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = FATSPANK;
	sprite[j].shade = -15;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short)(((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + (krand() & 15)
		- 8) + 2048) & 2047);
	sprite[j].xvel = bcos(sprite[j].ang, -6);
	sprite[j].yvel = bsin(sprite[j].ang, -6);
	long discrim = ksqrt((plr.x - sprite[j].x) * (plr.x - sprite[j].x) + (plr.y - sprite[j].y) * (plr.y - sprite[j].y));
	if (discrim == 0)
		discrim = 1;
	sprite[j].zvel = (short)(((plr.z + (48 << 8) - sprite[j].z) << 7) / discrim);
	sprite[j].owner = (short)i;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 512;
	sprite[j].hitag = 0;
	sprite[j].pal = 0;
}

void createFatwitchAI() {
	auto& e = enemy[FATWITCHTYPE];
	e.info.Init(32, 32, 2048, 120, 0, 64, false, 280, 0);
	e.chase = chasefatwitch;
	e.resurect = resurectfatwitch;
	e.search = searchfatwitch;
	e.nuked = nukedfatwitch;
	e.pain = painfatwitch;
	e.face = facefatwitch;
	e.attack = attackfatwitch;
	e.flee = fleefatwitch;
	e.cast = castfatwitch;
	e.die = diefatwitch;
}

void premapFatwitch(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = FATWITCHTYPE;
	changespritestat(i, FACE);
	enemy[FATWITCHTYPE].info.set(spr);
	if (spr.pal == 7)
		spr.hitag = (short)adjusthp(290);
	if (krand() % 100 > 50)
		spr.extra = 1;

}

END_WH_NS
