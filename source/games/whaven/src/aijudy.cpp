#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

void spawnabaddy(DWHActor* i, int monster);

static void chasejudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (mapon < 24) {
		spr.extra -= TICSPERFRAME;
		if (spr.extra < 0) {
			for (int j = 0; j < 8; j++)
				trailingsmoke(i, true);
			deletesprite((short)i);
			return;
		}
	}

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
			spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum))// && invisibletime < 0)
			SetNewStatus(actor, ATTACK);
	}
	else {
		checksight(plr, i);
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
}
	
static void resurectjudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		spr.picnum = JUDY;
		spr.hitag = (short)adjusthp(200);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void searchjudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();

	aisearch(plr, i, false);
	checksector6(i);
}
	
static void nukedjudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == JUDYCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}
	
static void painjudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = JUDY;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}
	
static void facejudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
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
	
static void attackjudy(PLAYER& plr, DWHActor* actor)
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

	setsprite(i, spr.x, spr.y, spr.z);

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
	
static void fleejudy(PLAYER& plr, DWHActor* actor)
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
}
	
static void castjudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == JUDYATTACK1 + 3) {
		spr.picnum = JUDYATTACK1;
		spritesound(S_JUDY1 + krand() % 4, &sprite[i]);
		if (krand() % 100 > 70) {
			castspell(plr, i);
		}
		else {
			if (krand() % 100 > 40) {
				// raise the dead
				WHStatIterator it(DEAD);
				while (auto itActor = it.Next())
				{
					SPRITE& spr = itActor->s();
					int j = itActor->GetSpriteIndex();

					spr.lotag = (short)((krand() % 120) + 120);
					kills--;
					newstatus(j, RESURECT);
				}
			}
			else {
				if (krand() % 100 > 50) {
					// curse
					for (int j = 1; j < 9; j++) {
						plr.ammo[j] = 3;
					}
				}
				else {
					int j = krand() % 5;
					switch (j) {
					case 0:// SPAWN WILLOW
						spawnabaddy(actor, WILLOW);
						break;
					case 1:// SPAWN 10 SPIDERS
						for (j = 0; j < 4; j++) {
							spawnabaddy(actor, SPIDER);
						}
						break;
					case 2:// SPAWN 2 GRONSW
						for (j = 0; j < 2; j++) {
							spawnabaddy(actor, GRONSW);
						}
						break;
					case 3:// SPAWN SKELETONS
						for (j = 0; j < 4; j++) {
							spawnabaddy(actor, SKELETON);
						}
						break;
					case 4:
						castspell(plr, i);
						break;
					}
				}
			}
		}
		SetNewStatus(actor, CHASE);
	}
	else if (spr.picnum == JUDYATTACK2 + 8) {
		spr.picnum = JUDYATTACK2;
		spritesound(S_JUDY1 + krand() % 4, &sprite[i]);
		if (krand() % 100 > 50)
			skullycastspell(plr, i);
		else {
			if (krand() % 100 > 70) {
				if (krand() % 100 > 50) {
					plr.health = 0;
					addhealth(plr, 1);
				}
				else {
					addarmor(plr, -(plr.armor));
					plr.armortype = 0;
				}
			}
			else {
				int j = krand() % 5;
				switch (j) {
				case 0:// SPAWN WILLOW
					spawnabaddy(actor, WILLOW);
					break;
				case 1:// SPAWN 6 SPIDERS
					for (j = 0; j < 4; j++) {
						spawnabaddy(actor, SPIDER);
					}
					break;
				case 2:// SPAWN 2 GRONSW
					for (j = 0; j < 2; j++) {
						spawnabaddy(actor, GRONSW);
					}
					break;
				case 3:// SPAWN SKELETONS
					for (j = 0; j < 4; j++) {
						spawnabaddy(actor, SKELETON);
					}
					break;
				case 4:
					castspell(plr, i);
					break;
				}
			}

		}
		SetNewStatus(actor, CHASE);
	}
	checksector6(i);
}
	
static void diejudy(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == JUDYDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}
	
void judyOperate(PLAYER& plr)
{
	WHStatIterator it(WITCHSIT);
	while (auto actor = it.Next())
	{
		SPRITE& spri = actor->s();
		int i = actor->GetSpriteIndex();


		spri.ang = (short)(getangle(plr.x - spri.x, plr.y - spri.y) & 2047);
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spri.x, spri.y,
			spri.z - (tileHeight(spri.picnum) << 7), spri.sectnum)) {
			spri.lotag -= TICSPERFRAME;
			if (spri.lotag < 0) {
				spri.picnum++;
				spri.lotag = 12;
				if (spri.picnum == JUDYSIT + 4) {
					spri.picnum = JUDY;
					SetNewStatus(actor, FACE);
				}
			}
		}
	}
}
	
void spawnabaddy(DWHActor* actor, int monster) {
	auto& spr = actor->s();
	short j = insertsprite(spr.sectnum, FACE);
	auto& spawned = sprite[j];

	spawned.x = spr.x + (krand() & 2048) - 1024;
	spawned.y = spr.y + (krand() & 2048) - 1024;
	spawned.z = spr.z;

	spawned.pal = 0;
	spawned.shade = 0;
	spawned.cstat = 0;

	if (monster == WILLOW)
		premapWillow(j);
	else if (monster == SPIDER)
		premapSpider(j);
	else if (monster == GRONSW)
		premapGron(j);
	else if (monster == SKELETON)
		premapSkeleton(j);
	else if (monster == GONZOGSH)
		premapGonzo(j);

	spawned.picnum = (short)monster;
	killcnt++;

	setsprite(j, spawned.x, spawned.y, spawned.z);
}


void createJudyAI() {
	auto& e = enemy[JUDYTYPE];
	e.info.Init(32, 32, 2048, 120, 0, 64, false, 500, 0);
	e.chase = chasejudy;
	e.resurect = resurectjudy;
	e.search = searchjudy;
	e.nuked = nukedjudy;
	e.pain = painjudy;
	e.face = facejudy;
	e.attack = attackjudy;
	e.flee = fleejudy;
	e.cast = castjudy;
	e.die = diejudy;
}

void premapJudy(short i) {
	SPRITE& spr = sprite[i];
	spr.detail = JUDYTYPE;

	enemy[JUDYTYPE].info.set(spr);

	if (mapon > 24)
		spr.hitag = adjusthp(700);

	if (spr.picnum == JUDYSIT) {
		changespritestat(i, WITCHSIT);
		spr.extra = 1200;
	}
	else
		changespritestat(i, FACE);
}

END_WH_NS
