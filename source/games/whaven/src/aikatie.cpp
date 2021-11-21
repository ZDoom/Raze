#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

void spawnabaddy(DWHActor* actor, int monster);

static void chasekatie(PLAYER& plr, DWHActor* actor)
{
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
		if (!checkdist(actor, plr.x, plr.y, plr.z)) {
			auto moveStat = aimove(actor);
			if (moveStat.type == kHitFloor)
			{
				spr.ang = ((spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
				return;
			}
		}
		else {
			if (krand() % 8 == 0) // NEW
				SetNewStatus(actor, ATTACK); // NEW
			else { // NEW
				spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE); // NEW
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
	
static void resurectkatie(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = KATIE;
		spr.hitag = adjusthp(200);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void searchkatie(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	checksector6(actor);
}
	
static void painkatie(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = KATIE;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);
}
	
static void facekatie(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.ang = (getangle(plr.x - spr.x, plr.y - spr.y) & 2047);

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
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
		else if (cansee) SetNewStatus(actor, FLEE);
	}

	if (checkdist(actor, plr.x, plr.y, plr.z))
		SetNewStatus(actor, ATTACK);
}
	
static void attackkatie(PLAYER& plr, DWHActor* actor)
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
	
static void fleekatie(PLAYER& plr, DWHActor* actor)
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

	if (moveStat.type != kHitNone) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		SetNewStatus(actor, FACE);
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
	
static void castkatie(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == KATIEAT + 2) {
		if (spr.extra >= 2) {
			if (rand() % 100 > 50) {
				for (int j = 0; j < 4; j++) {
					spawnabaddy(actor, GONZOGSH);
				}
			}
			else {
				for (int j = 0; j < 6; j++) {
					spawnabaddy(actor, GRONSW);
				}
			}
			spr.picnum = KATIE;
			spr.extra--;
			spritesound(S_FIREBALL, actor);
			SetNewStatus(actor, CHASE);
		}
	}

	if (spr.picnum == KATIEAT + 6) {
		if (spr.extra == 1) {
			WHSpriteIterator it;
			while (auto itActor = it.Next())
			{
				auto& spk = itActor->s();
				if (spk.pal == 8) {
					spk.picnum--;
					spk.pal = 0;
					spk.shade = 0;
					ChangeActorStat(itActor, FACE);
				}
			}
			spr.picnum = KATIE;
			spritesound(S_FIREBALL, actor);
			SetNewStatus(actor, CHASE);
			spr.extra--;
		}
	}

	if (spr.picnum == KATIEAT + 16) {
		spr.picnum = KATIE;
		spritesound(S_FIREBALL, actor);
		castspell(plr, actor);
		SetNewStatus(actor, CHASE);
		spr.extra++;
	}
	checksector6(actor);
}
	
static void diekatie(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();
	
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == KATIEDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

void createKatieAI() {
	auto& e = enemy[KATIETYPE];
	e.info.Init(35, 35, 2048, 120, 0, 64, false, 200, 0);
	e.chase = chasekatie;
	e.resurect = resurectkatie;
	e.search = searchkatie;
	e.pain = painkatie;
	e.face = facekatie;
	e.attack = attackkatie;
	e.flee = fleekatie;
	e.cast = castkatie;
	e.die = diekatie;
}

void premapKatie(DWHActor* actor) {
	SPRITE& spr = actor->s();

	spr.detail = KATIETYPE;
	ChangeActorStat(actor, FACE);
	enemy[KATIETYPE].info.set(spr);
	spr.extra = 5;
}

END_WH_NS
