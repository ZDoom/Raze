#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void chasedemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	if (plr.z < spr.z) {
		spr.z -= TICSPERFRAME << 8;
	}
	if (plr.z > spr.z) {
		spr.z += TICSPERFRAME << 8;
	}

	short osectnum = spr.sectnum;
	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum) && plr.invisibletime < 0)
			newstatus(i, ATTACK);
		return;
	}
	else {
		if (PlayClock % 100 > 70)
			trailingsmoke(i, true);

		int dax = (bcos(sprite[i].ang) * TICSPERFRAME) << 2;
		int day = (bsin(sprite[i].ang) * TICSPERFRAME) << 2;
		checksight(plr, i);


		if (!checkdist(plr, i)) {
			checkmove(i, dax, day);
		}
		else {
			if (plr.invisibletime < 0) {
				if (krand() % 8 == 0) // NEW
					newstatus(i, ATTACK); // NEW
				else { // NEW
					sprite[i].ang = (short)(((krand() & 512 - 256) + sprite[i].ang + 1024) & 2047); // NEW
					newstatus(i, CHASE); // NEW
				}
			}
		}
	}

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	checksector6(i);

	processfluid(i, zr_florhit, true);

	if (sector[osectnum].lotag == KILLSECTOR && spr.z + (8 << 8) >= sector[osectnum].floorz) {
		spr.hitag--;
		if (spr.hitag < 0)
			newstatus(i, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		if (spr.z + (8 << 8) >= sector[osectnum].floorz) {
			spr.hitag--;
			if (spr.hitag < 0)
				newstatus(i, DIE);
		}
	}
}

static void searchdemon(PLAYER& plr, short i) {
	aisearch(plr, i, true);
	checksector6(i);
}

static void paindemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = DEMON;
		spr.ang = plr.angle.ang.asbuild();
		newstatus(i, FLEE);
	}

	aifly(i);
	setsprite(i, spr.x, spr.y, spr.z);
}

static void facedemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum);

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
}


static void attackdemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

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
			newstatus(i, CAST);
		else
			newstatus(i, CHASE);
	}
	else
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
}

static void fleedemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

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
			newstatus(i, FACE);
		}
	}
	if (spr.lotag < 0)
		newstatus(i, FACE);

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, true);

	setsprite(i, spr.x, spr.y, spr.z);
}

static void castdemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;

	if (plr.z < spr.z) {
		spr.z -= TICSPERFRAME << 8;
	}
	if (plr.z > spr.z) {
		spr.z += TICSPERFRAME << 8;
	}

	if (spr.lotag < 0) {
		castspell(plr, i);
		newstatus(i, CHASE);
	}
}

static void nukeddemon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(i, false);
	newstatus((short)i, DIE);
}


void createDemonAI() {
	auto& e = enemy[DEMONTYPE];
	enemy[DEMONTYPE].info.Init(38, 41, 4096 + 2048, 120, 0, 64, true, 300, 0);

	e.chase = chasedemon;
	e.resurect;
	e.nuked = nukeddemon;
	e.frozen;
	e.pain = paindemon;
	e.face = facedemon;
	e.attack = attackdemon;
	e.flee = fleedemon;
	e.cast = castdemon;
	e.die;
	e.skirmish;
	e.stand;
	e.search = searchdemon;
}


void premapDemon(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = DEMONTYPE;
	changespritestat(i, FACE);
	enemy[DEMONTYPE].info.set(spr);
}

END_WH_NS
