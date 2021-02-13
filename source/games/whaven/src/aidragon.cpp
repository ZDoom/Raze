#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static int checksight_x, checksight_y = 0;

static void checkspeed(int i, int speed);
static void dragonAttack2(PLAYER& plr, short i);
static void firebreath(PLAYER& plr, int i, int a, int b, int c);

static void chasedragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	//				int speed = 10;
	if ((krand() % 16) == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0)
			if (plr.z < spr.z)
				newstatus(i, ATTACK2);
			else
				newstatus(i, ATTACK);
		return;
	}
	else {
		int dax = (bcos(sprite[i].ang) * TICSPERFRAME) << 3;
		int day = (bsin(sprite[i].ang) * TICSPERFRAME) << 3;
		//					checkspeed(i, speed);
		checksight(plr, i);
		if (!checkdist(plr, i)) {
			//						checkmove(i, checksight_x, checksight_y);
			checkmove(i, dax, day);
		}
		else {
			if (plr.invisibletime < 0) {
				if (krand() % 8 == 0) { // NEW
					if (plr.z < spr.z)
						newstatus(i, ATTACK2);
					else
						newstatus(i, ATTACK);
				}
				else { // NEW
					newstatus(i, FACE); // NEW
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

static void fleedragon(PLAYER& plr, short i) {
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

static void diedragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;
		if (spr.picnum == DRAGONDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
		}
	}
}

static void castdragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	switch (spr.picnum) {
	case DRAGONATTACK + 17:
	case DRAGONATTACK + 4:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, &spr);
		else
			spritesound(S_FIREBALL, &spr);

		firebreath(plr, i, 1, 2, LOW);
		break;
	case DRAGONATTACK + 18:
	case DRAGONATTACK + 5:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, &spr);
		else
			spritesound(S_FIREBALL, &spr);

		firebreath(plr, i, 2, 1, LOW);
		break;
	case DRAGONATTACK + 19:
	case DRAGONATTACK + 6:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, &spr);
		else
			spritesound(S_FIREBALL, &spr);

		firebreath(plr, i, 4, 0, LOW);
		break;
	case DRAGONATTACK + 20:
	case DRAGONATTACK + 7:
		firebreath(plr, i, 2, -1, LOW);
		break;
	case DRAGONATTACK + 21:
	case DRAGONATTACK + 8:
		firebreath(plr, i, 1, -2, LOW);
		break;

	case DRAGONATTACK2 + 2:
		if ((krand() % 2) != 0)
			spritesound(S_FLAME1, &spr);
		else
			spritesound(S_FIREBALL, &spr);

		firebreath(plr, i, 1, -1, HIGH);
		break;
	case DRAGONATTACK2 + 3:
		firebreath(plr, i, 2, 0, HIGH);
		break;

	case DRAGONATTACK2 + 5:
		spr.picnum = DRAGON;
		newstatus(i, CHASE);
		break;
	case DRAGONATTACK + 22:
		spr.picnum = DRAGONATTACK;
		newstatus(i, CHASE);
		break;
	}

	checksector6(i);
}

static void attackdragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

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

static void resurectdragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		spr.picnum = DRAGON;
		spr.hitag = (short)adjusthp(900);
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}

static void searchdragon(PLAYER& plr, short i) {
	aisearch(plr, i, true);
	checksector6(i);
}

static void frozendragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		spr.picnum = DRAGON;
		newstatus(i, FACE);
	}
}

static void nukeddragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 24;
		if (spr.picnum == DRAGONCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}

static void paindragon(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}

static void facedragon(PLAYER& plr, short i) {
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


void dragonProcess(PLAYER& plr)
{
	for (short i = headspritestat[ATTACK2], nextsprite; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		switch (spr.detail) {
		case DRAGON:
			dragonAttack2(plr, i);
			break;
		}
	}
}

static void dragonAttack2(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0)
			newstatus(i, CAST);
		else
			newstatus(i, CHASE);
		return;
	}
	else
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);

	checksector6(i);
}

static void firebreath(PLAYER& plr, int i, int a, int b, int c) {
	for (int k = 0; k <= a; k++) {
		int j = insertsprite(sprite[i].sectnum, MISSILE);
		if (j == -1)
			return;

		sprite[j].x = sprite[i].x;
		sprite[j].y = sprite[i].y;
		if (c == LOW)
			sprite[j].z = sector[sprite[i].sectnum].floorz - (32 << 8);
		else
			sprite[j].z = sector[sprite[i].sectnum].floorz - (tileHeight(sprite[i].picnum) << 7);
		sprite[j].cstat = 0;
		sprite[j].picnum = MONSTERBALL;
		sprite[j].shade = -15;
		sprite[j].xrepeat = 128;
		sprite[j].yrepeat = 128;
		sprite[j].ang = (short)((((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y)
			+ (krand() & 15) - 8) + 2048) + ((b * 22) + (k * 10))) & 2047);
		sprite[j].xvel = bcos(sprite[j].ang, -6);
		sprite[j].yvel = bsin(sprite[j].ang, -6);
		int discrim = ksqrt(
			(plr.x - sprite[j].x) * (plr.x - sprite[j].x) + (plr.y - sprite[j].y) * (plr.y - sprite[j].y));
		if (discrim == 0)
			discrim = 1;
		if (c == HIGH)
			sprite[j].zvel = (short)(((plr.z + (32 << 8) - sprite[j].z) << 7) / discrim);
		else
			sprite[j].zvel = (short)((((plr.z + (8 << 8)) - sprite[j].z) << 7) / discrim);// NEW

		sprite[j].owner = (short)i;
		sprite[j].clipdist = 16;
		sprite[j].lotag = 512;
		sprite[j].hitag = 0;
	}
}
	
static void checkspeed(int i, int speed) {
	checksight_x = bcos(sprite[i].ang, -speed);
	checksight_y = bsin(sprite[i].ang, -speed);
}


void createDragonAI() {
	auto& e = enemy[DRAGONTYPE];
	e.info.Init(54, 54, 512, 120, 0, 128, false, 900, 0);
	e.chase = chasedragon;
	e.flee = fleedragon;
	e.die = diedragon;
	e.cast = castdragon;
	e.attack = attackdragon;
	e.resurect = resurectdragon;
	e.search = searchdragon;
	e.frozen = frozendragon;
	e.nuked = nukeddragon;
	e.pain = paindragon;
	e.face = facedragon;
}

void premapDragon(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = DRAGONTYPE;
	changespritestat(i, FACE);
	enemy[DRAGONTYPE].info.set(spr);
}

END_WH_NS
