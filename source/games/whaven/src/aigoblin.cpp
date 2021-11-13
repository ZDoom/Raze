#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexplgoblin(PLAYER& plr, DWHActor* i);


static void chasegoblin(PLAYER& plr, DWHActor* actor)
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

	setsprite(i, spr.x, spr.y, spr.z);

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
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
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = GOBLIN;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);

	checkexplgoblin(plr, actor);
}

static void facegoblin(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
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

	if (checkdist(plr, i))
		SetNewStatus(actor, ATTACK);

	checkexplgoblin(plr, actor);
}

static void fleegoblin(PLAYER& plr, DWHActor* actor)
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

	setsprite(i, spr.x, spr.y, spr.z);

	checkexplgoblin(plr, actor);
}

static void standgoblin(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			switch (spr.picnum) {
			case GOBLINCHILL:
				spr.picnum = GOBLINSURPRISE;
				spritesound(S_GOBPAIN1 + (krand() % 2), &spr);
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

	if (spr.lotag == 31) {
		if (checksight(plr, actor))
			if (checkdist(plr, i)) {
				spr.ang = (short)checksight_ang;
				attack(plr, i);
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
	int i = actor->GetSpriteIndex();

	aisearch(plr, i, false);
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
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == GOBLINCHAR + 4) {
			trailingsmoke(actor,false);
			deletesprite(i);
		}
	}
}

static void skirmishgoblin(PLAYER& plr, DWHActor* actor)
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
		warpsprite(actor);

	processfluid(i, zr_florhit, false);

	setsprite(i, spr.x, spr.y, spr.z);

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
	int i = actor->GetSpriteIndex();
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

		int movehit = aimove(i);
		if (movehit == 0)
			spr.ang = getangle(ownerspr.x - spr.x, ownerspr.y - spr.y);
		else if ((movehit & kHitTypeMask) == kHitWall) {
			spr.extra = 3;
			spr.ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
			spr.lotag = 60;
		}
		else if ((movehit & kHitTypeMask) == kHitSprite) {
			int sprnum = movehit & kHitIndexMask;
			if (sprnum != k) {
				spr.extra = 3;
				spr.ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
				spr.lotag = 60;
			}
			else spr.ang = getangle(ownerspr.x - spr.x, ownerspr.y - spr.y);
		}

		processfluid(i, zr_florhit, false);

		setsprite(i, spr.x, spr.y, spr.z);
		if (checkdist(i, ownerspr.x, ownerspr.y, ownerspr.z)) {
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
		if (checkdist(i, ownerspr.x, ownerspr.y, ownerspr.z)) {
			if ((krand() & 1) != 0) {
				// goblins are fighting
				// JSA_DEMO
				if (krand() % 10 > 6)
					spritesound(S_GENSWING, &spr);
				if (krand() % 10 > 6)
					spritesound(S_SWORD1 + (krand() % 6), &spr);

				if (checkdist(plr, i))
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

		processfluid(i, zr_florhit, false);

		setsprite(i, spr.x, spr.y, spr.z);

		if (checksector6(actor))
			return;

		break;
	}
	case 3: // flee
		spr.lotag -= TICSPERFRAME;

		if (aimove(i) != 0)
			spr.ang = (short)(krand() & 2047);
		processfluid(i, zr_florhit, false);

		setsprite(i, spr.x, spr.y, spr.z);

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
	int i = actor->GetSpriteIndex();
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


void premapGoblin(short i) {
	SPRITE& spr = sprite[i];

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
		changespritestat(i, STAND);
		spr.lotag = 30;
		if (krand() % 100 > 50)
			spr.extra = 1;
		return;
	}

	changespritestat(i, FACE);
	if (krand() % 100 > 50)
		spr.extra = 1;
}

END_WH_NS
