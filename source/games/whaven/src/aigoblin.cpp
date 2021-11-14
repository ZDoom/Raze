#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexplgoblin(PLAYER& plr, DWHActor* actor);


static void chasegoblin(PLAYER& plr, DWHActor* actor)
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

	checkexplgoblin(plr, actor);
}
		
static void diegoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == GOBLINDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}

static void paingoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = GOBLIN;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);

	checkexplgoblin(plr, actor);
}

static void facegoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		spr.picnum = GOBLIN;
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

	checkexplgoblin(plr, actor);
}

static void fleegoblin(PLAYER& plr, DWHActor* actor)
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

	checkexplgoblin(plr, actor);
}

static void standgoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			switch (spr.picnum) {
			case GOBLINCHILL:
				spr.picnum = GOBLINSURPRISE;
				spritesound(S_GOBPAIN1 + (krand() % 2), actor);
				SetNewStatus(actor, CHILL);
				break;
			default:
				spr.picnum = GOBLIN;
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
					SetNewStatus(actor, FLEE);
				}
				else
					SetNewStatus(actor, CHASE);
				break;
			}
		}
	}

	checksector6(actor);
}
		
static void attackgoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

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
				attack(plr, actor);
			}
	}
	else if (spr.lotag < 0) {
		spr.picnum = GOBLIN;
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

static void resurectgoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = GOBLIN;
		spr.hitag = (short)adjusthp(35);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void searchgoblin(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	if (!checksector6(actor))
		checkexplgoblin(plr, actor);
}
		
static void frozengoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = GOBLIN;
		SetNewStatus(actor, FACE);
	}
}

static void nukedgoblin(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == GOBLINCHAR + 4) {
			trailingsmoke(actor,false);
			DeleteActor(actor);
		}
	}
}

static void skirmishgoblin(PLAYER& plr, DWHActor* actor)
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

	checkexplgoblin(plr, actor);
}

void goblinChill(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 18;
		if (spr.picnum == GOBLINSURPRISE + 5) {
			spr.picnum = GOBLIN;
			SetNewStatus(actor, FACE);
		}
	}
}

static void goblinWar(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	short k;

	if (spr.lotag > 256) {
		spr.lotag = 100;
		spr.extra = 0;
	}

	switch (spr.extra) {
	case 0: // find new target
	{
		int olddist = 1024 << 4;
		boolean found = false;
		WHSpriteIterator it;
		while (auto itActor = it.Next())
		{
			auto& spk = itActor->s();
			if (spk.picnum == GOBLIN && spr.pal != spk.pal && spr.hitag == spk.hitag) {
				int dist = abs(spr.x - spk.x) + abs(spr.y - spk.y);
				if (dist < olddist) {
					found = true;
					olddist = dist;
					spr.owner = itActor->GetSpriteIndex();
					spr.ang = getangle(spk.x - spr.x, spk.y - spr.y);
					spr.extra = 1;
				}
			}
		}
		if (!found) {
			if (spr.pal == 5)
				spr.hitag = (short)adjusthp(35);
			else if (spr.pal == 4)
				spr.hitag = (short)adjusthp(25);
			else
				spr.hitag = (short)adjusthp(15);
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, FACE);
		}
		break;
	}
	case 1: // chase
	{
		k = spr.owner;
		auto owneractor = &whActors[k];
		auto ownerspr = owneractor->s();

		auto moveStat = aimove(actor);
		if (moveStat.type == kHitNone)
			spr.ang = getangle(ownerspr.x - spr.x, ownerspr.y - spr.y);
		else if (moveStat.type == kHitWall) {
			spr.extra = 3;
			spr.ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
			spr.lotag = 60;
		}
		else if (moveStat.type == kHitSprite) {
			int sprnum = moveStat.actor->GetSpriteIndex();
			if (sprnum != k) {
				spr.extra = 3;
				spr.ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
				spr.lotag = 60;
			}
			else spr.ang = getangle(ownerspr.x - spr.x, ownerspr.y - spr.y);
		}

		processfluid(actor, zr_florHit, false);

		SetActorPos(actor, &spr.pos);
		if (checkdist(actor, ownerspr.x, ownerspr.y, ownerspr.z)) {
			spr.extra = 2;
		}
		else
			spr.picnum = GOBLIN;

		if (checksector6(actor))
			return;

		break;
	}
	case 2: // attack
	{
		k = spr.owner;
		auto owneractor = &whActors[k];
		auto& ownerspr = owneractor->s();
		if (checkdist(actor, ownerspr.x, ownerspr.y, ownerspr.z)) {
			if ((krand() & 1) != 0) {
				// goblins are fighting
				// JSA_DEMO
				if (krand() % 10 > 6)
					spritesound(S_GENSWING, actor);
				if (krand() % 10 > 6)
					spritesound(S_SWORD1 + (krand() % 6), actor);

				if (checkdist(plr, actor))
					addhealth(plr, -(krand() & 5));

				if (krand() % 100 > 90) { // if k is dead
					spr.extra = 0; // needs to
					spr.picnum = GOBLIN;
					ownerspr.extra = 4;
					ownerspr.picnum = GOBLINDIE;
					ownerspr.lotag = 20;
					ownerspr.hitag = 0;
					SetNewStatus(owneractor, DIE);
				}
				else { // i attack k flee
					spr.extra = 0;
					ownerspr.extra = 3;
					ownerspr.ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
					ownerspr.lotag = 60;
				}
			}
		}
		else {
			spr.extra = 1;
		}

		processfluid(actor, zr_florHit, false);

		SetActorPos(actor, &spr.pos);

		if (checksector6(actor))
			return;

		break;
	}
	case 3: // flee
		spr.lotag -= TICSPERFRAME;

		if (aimove(actor).type != kHitNone)
			spr.ang = (short)(krand() & 2047);
		processfluid(actor, zr_florHit, false);

		SetActorPos(actor, &spr.pos);

		if (spr.lotag < 0) {
			spr.lotag = 0;
			spr.extra = 0;
		}

		if (checksector6(actor))
			return;

		break;
	case 4: // pain
		spr.picnum = GOBLINDIE;
		break;
	case 5: // cast
		break;
	}

	checkexplgoblin(plr, actor);
}
	
void goblinWarProcess(PLAYER& plr)
{
	WHStatIterator it(WAR);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		switch (spr.detail) {
		case GOBLINTYPE:
			goblinWar(plr, actor);
			break;
		}
	}
}

static void checkexplgoblin(PLAYER& plr, DWHActor* actor)
{
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
				|| spri.picnum == SMOKEFX
				|| spri.picnum == MONSTERBALL) {
				spr.hitag -= TICSPERFRAME << 2;
				if (spr.hitag < 0) {
					SetNewStatus(actor, DIE);
				}
			}
		}
	}
}

void createGoblinAI() {
	auto& e = enemy[GOBLINTYPE];
	e.info.Init(36, 36, 1024, 120, 0, 64, false, 15, 0);
	e.chase = chasegoblin;
	e.die = diegoblin;
	e.pain = paingoblin;
	e.face = facegoblin;
	e.flee = fleegoblin;
	e.stand = standgoblin;
	e.attack = attackgoblin;
	e.resurect = resurectgoblin;
	e.search = searchgoblin;
	e.frozen = frozengoblin;
	e.nuked = nukedgoblin;
	e.skirmish = skirmishgoblin;
	e.info.getHealth = [](EnemyInfo& e, SPRITE& spr)
	{
		if (spr.pal == 5)
			return adjusthp(35);
		else if (spr.pal == 4)
			return adjusthp(25);

		return adjusthp(e.health);
	};
}


void premapGoblin(DWHActor* actor) {
	SPRITE& spr = actor->s();

	spr.detail = GOBLINTYPE;

	if (spr.hitag < 90 || spr.hitag > 99)
		enemy[GOBLINTYPE].info.set(spr);
	else {
		short ohitag = spr.hitag;
		enemy[GOBLINTYPE].info.set(spr);
		if (spr.pal != 0)
			spr.xrepeat = 30;
		spr.extra = 0;
		spr.owner = 0;
		spr.hitag = ohitag;
		return;
	}

	if (spr.picnum == GOBLINCHILL) {
		ChangeActorStat(actor, STAND);
		spr.lotag = 30;
		if (krand() % 100 > 50)
			spr.extra = 1;
		return;
	}

	ChangeActorStat(actor, FACE);
	if (krand() % 100 > 50)
		spr.extra = 1;
}

END_WH_NS
