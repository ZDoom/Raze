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
			sprite[i].z - (tilesizy[sprite[i].picnum] << 7), sprite[i].sectnum))// && invisibletime < 0)
			newstatus(i, ATTACK);
	}
	else {
		checksight(plr, i);
		if (!checkdist(i, plr.x, plr.y, plr.z)) {
			int movestat = aimove(i);
			if ((movestat & kHitTypeMask) == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
				return;
			}
		}
		else {
			if (krand() % 8 == 0) // NEW
				newstatus(i, ATTACK); // NEW
			else { // NEW
				sprite[i].ang = (short)(((krand() & 512 - 256) + sprite[i].ang + 1024) & 2047); // NEW
				newstatus(i, FLEE); // NEW
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
	
static void resurect(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		spr.picnum = FATWITCH;
		spr.hitag = (short)adjusthp(90);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void search(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	checksector6(i);
}
	
static void nuked(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == FATWITCHCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}
	
static void pain(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = FATWITCH;
		spr.ang = (short)plr.ang;
		newstatus(i, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}
	
static void face(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	boolean cansee = cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tilesizy[spr.picnum] << 7), spr.sectnum);

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

	if (checkdist(i, plr.x, plr.y, plr.z))
		newstatus(i, ATTACK);
}
	
static void attack(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
	case TYPEWATER:
		spr.z += tilesizy[spr.picnum] << 5;
		break;
	}

	setsprite(i, spr.x, spr.y, spr.z);

	sprite[i].lotag -= TICSPERFRAME;
	if (sprite[i].lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tilesizy[sprite[i].picnum] << 7), sprite[i].sectnum))
			newstatus(i, CAST);
		else
			newstatus(i, CHASE);
	}
	else
		sprite[i].ang = getangle(plr.x - sprite[i].x, plr.y - sprite[i].y);
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
	
static void cast(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == FATWITCHATTACK + 3) {
		sprite[i].picnum = FATWITCH;
		throwspank(plr, i);
		newstatus(i, CHASE);
	}
	checksector6(i);
}
	
static void die(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == FATWITCHDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
		}
	}
}


void throwspank(PLAYER& plr, int i) {
	int j = insertsprite(sprite[i].sectnum, MISSILE);
	if (j == -1)
		return;
	playsound_loc(S_WITCHTHROW, sprite[i].x, sprite[i].y);

	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sector[sprite[i].sectnum].floorz - ((tilesizy[sprite[i].picnum] >> 1) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = FATSPANK;
	sprite[j].shade = -15;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short)(((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + (krand() & 15)
		- 8) + 2048) & 2047);
	sprite[j].xvel = (short)(sintable[(sprite[j].ang + 2560) & 2047] >> 6);
	sprite[j].yvel = (short)(sintable[(sprite[j].ang + 2048) & 2047] >> 6);
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
	e.chase = chase;
	e.resurect = resurect;
	e.search = search;
	e.nuked = nuked;
	e.pain = pain;
	e.face = face;
	e.attack = attack;
	e.flee = flee;
	e.cast = cast;
	e.die = die;
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
