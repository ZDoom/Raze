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
	spr.z = zr_florz;

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
	
static void resurect(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		spr.picnum = MINOTAUR;
		spr.hitag = (short)adjusthp(100);
		spr.lotag = 100;
		spr.cstat |= 1;
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
	
static void nuked(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	if (isWh2()) {
		chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
		trailingsmoke(i, false);
		newstatus((short)i, DIE);
		return;
	}

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == MINOTAURCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}
	
static void pain(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = MINOTAUR;
		spr.ang = (short)plr.ang;
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
		spr.ang = (short)(getangle(plr.x - spr.x, plr.y - spr.y) & 2047);

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
	
static void attack(PLAYER& plr, short i) {
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
	
static void die(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == MINOTAURDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
		}
	}
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
	
static void frozen(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = MINOTAUR;
		newstatus(i, FACE);
	}
}
	
static void search(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	if (!checksector6(i))
		checkexpl(plr, i);
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

void createMinotaurAI() {

	//picanm[MINOTAUR + 16] = 0;

	auto& e = enemy[MINOTAURTYPE];
	e.info.Init(64, 64, 1024 + 512, 120, 0, 64, false, isWh2() ? 80 : 100, 0);
	e.chase = chase;
	e.resurect = resurect;
	e.skirmish = skirmish;
	e.nuked = nuked;
	e.pain = pain;
	e.face = face;
	e.attack = attack;
	e.die = die;
	e.flee = flee;
	e.frozen = frozen;
	e.search = search;
}


void premapMinotaur(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = MINOTAURTYPE;
	enemy[MINOTAURTYPE].info.set(spr);
	changespritestat(i, FACE);

	if (krand() % 100 > 50)
		spr.extra = 1;
}

END_WH_NS
