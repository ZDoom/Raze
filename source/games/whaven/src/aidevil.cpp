#include "ns.h"
#include "wh.h"

BEGIN_WH_NS


static void chase(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum) && plr.invisibletime < 0)
			newstatus(i, ATTACK);
	}
	else {
		checksight(plr, i);
		if (!checkdist(plr, i)) {
			if ((aimove(i) & kHitTypeMask) == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
				return;
			}
		}
		else {
			if (plr.invisibletime < 0) {
				if (krand() % 8 == 0) // NEW
					newstatus(i, ATTACK); // NEW
				else { // NEW
					sprite[i].ang = (short)(((krand() & 512 - 256) + sprite[i].ang + 1024) & 2047); // NEW
					newstatus(i, FLEE); // NEW
				}
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
			newstatus(i, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);
}

static void die(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == DEVILDEAD) {
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
		spr.picnum = DEVIL;
		spr.ang = (short)plr.ang;
		newstatus(i, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}

static void face(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];



	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
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
}

static void attack(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	setsprite(i, spr.x, spr.y, spr.z);

	sprite[i].extra -= TICSPERFRAME;
	sprite[i].lotag -= TICSPERFRAME;
	if (sprite[i].lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum))
			newstatus(i, CAST);
		else
			newstatus(i, CHASE);
	}
	else
		sprite[i].ang = getangle(plr.x - sprite[i].x, plr.y - sprite[i].y);
}

static void resurect(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		spr.picnum = DEVIL;
		spr.hitag = (short)adjusthp(60);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void search(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	checksector6(i);
}

static void frozen(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = DEVIL;
		newstatus(i, FACE);
	}
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
		if (spr.picnum == DEVILCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}

static void cast(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == DEVILATTACK + 2) {
		spr.picnum = DEVIL;
		playsound_loc(S_FIREBALL, sprite[i].x, sprite[i].y);
		castspell(plr, i);
		newstatus(i, CHASE);
	}
	checksector6(i);
}


void createDevilAI() {
	auto &e = enemy[DEVILTYPE];
	e.info.Init(isWh2() ? 50 : 36, isWh2() ? 50 : 36, 2048, 120, 0, 64, false, 50, 0);
	e.chase = chase;
	e.die = die;
	e.pain = pain;
	e.face = face;
	e.flee = flee;
	e.attack = attack;
	e.resurect = resurect;
	e.search = search;
	e.frozen = frozen;
	e.nuked = nuked;
	e.cast = cast;
}

void premapDevil(short i) {
	SPRITE& spr = sprite[i];
	spr.detail = DEVILTYPE;
	changespritestat(i, FACE);
	enemy[DEVILTYPE].info.set(spr);
	if (spr.pal == 2)
		spr.hitag = adjusthp(60);
}

END_WH_NS
