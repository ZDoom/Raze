#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexpl(PLAYER& plr, short i);


static void chase(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && plr.invisibletime < 0) {
		if (checkdist(plr, i)) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
			}
			else
				newstatus(i, ATTACK);
		}
		else if (krand() % 63 > 60) {
			spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
			newstatus(i, FLEE);
		}
		else {
			int movestat = aimove(i);
			if ((movestat & kHitTypeMask) == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
				return;
			}

			if ((movestat & kHitTypeMask) == kHitSprite) {
				if ((movestat & kHitIndexMask) != plr.spritenum) {
					short daang = (short)((spr.ang - 256) & 2047);
					spr.ang = daang;
					if (plr.shadowtime > 0) {
						spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						newstatus(i, FLEE);
					}
					else
						newstatus(i, SKIRMISH);
				}
				else {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					newstatus(i, SKIRMISH);
				}
			}
		}
	}
	else {
		spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
		newstatus(i, FLEE);
	}

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, false);

	if (sector[osectnum].lotag == KILLSECTOR) {
		spr.hitag--;
		if (spr.hitag < 0)
			newstatus(i, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			newstatus(i, DIE);
	}

	checkexpl(plr, i);
}
		
static void die(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == GOBLINDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
		}
	}
}

static void pain(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = GOBLIN;
		spr.ang = plr.angle.ang.asbuild();
		newstatus(i, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);

	checkexpl(plr, i);
}

static void face(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		spr.picnum = GOBLIN;
		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			newstatus(i, FLEE);
		}
		else {
			spr.owner = plr.spritenum;
			newstatus(i, CHASE);
		}
	}
	else { // get off the wall
		if (spr.owner == plr.spritenum) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang) & 2047);
			newstatus(i, FINDME);
		}
		else if (cansee) newstatus(i, FLEE);
	}

	if (checkdist(plr, i))
		newstatus(i, ATTACK);

	checkexpl(plr, i);
}

static void flee(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
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
			newstatus(i, FACE);
		}
	}
	if (spr.lotag < 0)
		newstatus(i, FACE);

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, false);

	setsprite(i, spr.x, spr.y, spr.z);

	checkexpl(plr, i);
}

static void stand(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	if (sintable[(spr.ang + 2560) & 2047] * (plr.x - spr.x)
		+ sintable[(spr.ang + 2048) & 2047] * (plr.y - spr.y) >= 0)
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			switch (spr.picnum) {
			case GOBLINCHILL:
				spr.picnum = GOBLINSURPRISE;
				spritesound(S_GOBPAIN1 + (krand() % 2), &spr);
				newstatus(i, CHILL);
				break;
			default:
				spr.picnum = GOBLIN;
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
					newstatus(i, FLEE);
				}
				else
					newstatus(i, CHASE);
				break;
			}
		}

	checksector6(i);
}
		
static void attackfunc(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
		sprite[i].hitag--;
		if (sprite[i].hitag < 0)
			newstatus(i, DIE);
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	setsprite(i, spr.x, spr.y, spr.z);

	if (spr.lotag == 31) {
		if (checksight(plr, i))
			if (checkdist(plr, i)) {
				spr.ang = (short)checksight_ang;
				attack(plr, i);
			}
	}
	else if (spr.lotag < 0) {
		spr.picnum = GOBLIN;
		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			newstatus(i, FLEE);
		}
		else
			newstatus(i, CHASE);
	}
	spr.lotag -= TICSPERFRAME;

	checksector6(i);
}

static void resurect(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		spr.picnum = GOBLIN;
		spr.hitag = (short)adjusthp(35);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void search(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	if (!checksector6(i))
		checkexpl(plr, i);
}
		
static void frozen(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = GOBLIN;
		newstatus(i, FACE);
	}
}

static void nuked(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == GOBLINCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}

static void skirmish(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag < 0)
		newstatus(i, FACE);
	short osectnum = spr.sectnum;
	int movestat = aimove(i);
	if ((movestat & kHitTypeMask) != kHitFloor && movestat != 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		newstatus(i, FACE);
	}
	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	processfluid(i, zr_florhit, false);

	setsprite(i, spr.x, spr.y, spr.z);

	if (checksector6(i))
		return;

	checkexpl(plr, i);
}

void goblinChill(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 18;
		if (spr.picnum == GOBLINSURPRISE + 5) {
			spr.picnum = GOBLIN;
			newstatus(i, FACE);
		}
	}
}

static void goblinWar(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
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
		for (k = 0; k < MAXSPRITES; k++) {
			if (sprite[k].picnum == GOBLIN && spr.pal != sprite[k].pal && spr.hitag == sprite[k].hitag) {
				int dist = abs(spr.x - sprite[k].x) + abs(spr.y - sprite[k].y);
				if (dist < olddist) {
					found = true;
					olddist = dist;
					spr.owner = k;
					spr.ang = getangle(sprite[k].x - spr.x, sprite[k].y - spr.y);
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
				newstatus(i, FLEE);
			}
			else
				newstatus(i, FACE);
		}
		break;
	}
	case 1: // chase
	{
		k = spr.owner;

		int movehit = aimove(i);
		if (movehit == 0)
			spr.ang = getangle(sprite[k].x - spr.x, sprite[k].y - spr.y);
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
			else spr.ang = getangle(sprite[k].x - spr.x, sprite[k].y - spr.y);
		}

		processfluid(i, zr_florhit, false);

		setsprite(i, spr.x, spr.y, spr.z);
		if (checkdist(i, sprite[k].x, sprite[k].y, sprite[k].z)) {
			spr.extra = 2;
		}
		else
			spr.picnum = GOBLIN;

		if (checksector6(i))
			return;

		break;
	}
	case 2: // attack
	{
		k = spr.owner;
		if (checkdist(i, sprite[k].x, sprite[k].y, sprite[k].z)) {
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
					sprite[k].extra = 4;
					sprite[k].picnum = GOBLINDIE;
					sprite[k].lotag = 20;
					sprite[k].hitag = 0;
					newstatus(k, DIE);
				}
				else { // i attack k flee
					spr.extra = 0;
					sprite[k].extra = 3;
					sprite[k].ang = (short)((spr.ang + (krand() & 256 - 128)) & 2047);
					sprite[k].lotag = 60;
				}
			}
		}
		else {
			spr.extra = 1;
		}

		processfluid(i, zr_florhit, false);

		setsprite(i, spr.x, spr.y, spr.z);

		if (checksector6(i))
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

		if (checksector6(i))
			return;

		break;
	case 4: // pain
		spr.picnum = GOBLINDIE;
		break;
	case 5: // cast
		break;
	}

	checkexpl(plr, i);
}
	
void goblinWarProcess(PLAYER& plr)
{
	for (short i = headspritestat[WAR], nextsprite; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		switch (spr.detail) {
		case GOBLINTYPE:
			goblinWar(plr, i);
			break;
		}
	}
}

static void checkexpl(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	short j = headspritesect[spr.sectnum];
	while (j != -1) {
		short nextj = nextspritesect[j];
		int dx = abs(spr.x - sprite[j].x); // x distance to sprite
		int dy = abs(spr.y - sprite[j].y); // y distance to sprite
		int dz = abs((spr.z >> 8) - (sprite[j].z >> 8)); // z distance to sprite
		int dh = tileHeight(sprite[j].picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (sprite[j].picnum == EXPLO2
				|| sprite[j].picnum == SMOKEFX
				|| sprite[j].picnum == MONSTERBALL) {
				spr.hitag -= TICSPERFRAME << 2;
				if (spr.hitag < 0) {
					newstatus(i, DIE);
				}
			}
		}
		j = nextj;
	}
}

void createGoblinAI() {
	auto& e = enemy[GOBLINTYPE];
	e.info.Init(36, 36, 1024, 120, 0, 64, false, 15, 0);
	e.chase = chase;
	e.die = die;
	e.pain = pain;
	e.face = face;
	e.flee = flee;
	e.stand = stand;
	e.attack = attackfunc;
	e.resurect = resurect;
	e.search = search;
	e.frozen = frozen;
	e.nuked = nuked;
	e.skirmish = skirmish;
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
