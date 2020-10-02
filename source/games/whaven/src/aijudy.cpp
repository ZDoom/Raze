#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

void spawnabaddy(int i, int monster);

static void chase(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (mapon < 24) {
		sprite[i].extra -= TICSPERFRAME;
		if (sprite[i].extra < 0) {
			for (int j = 0; j < 8; j++)
				trailingsmoke(i, true);
			deletesprite((short)i);
			return;
		}
	}

	if (krand() % 63 == 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum))// && invisibletime < 0)
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
		spr.picnum = JUDY;
		spr.hitag = (short)adjusthp(200);
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
		if (spr.picnum == JUDYCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}
	
static void pain(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = JUDY;
		spr.ang = (short)plr.ang;
		newstatus(i, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);
}
	
static void face(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
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

	if (spr.picnum == JUDYATTACK1 + 3) {
		sprite[i].picnum = JUDYATTACK1;
		playsound_loc(S_JUDY1 + krand() % 4, sprite[i].x, sprite[i].y);
		if (krand() % 100 > 70) {
			castspell(plr, i);
		}
		else {
			if (krand() % 100 > 40) {
				// raise the dead
				short j = headspritestat[DEAD];
				while (j >= 0) {
					short nextj = nextspritestat[j];
					sprite[j].lotag = (short)((krand() % 120) + 120);
					kills--;
					newstatus(j, RESURECT);
					j = nextj;
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
						spawnabaddy(i, WILLOW);
						break;
					case 1:// SPAWN 10 SPIDERS
						for (j = 0; j < 4; j++) {
							spawnabaddy(i, SPIDER);
						}
						break;
					case 2:// SPAWN 2 GRONSW
						for (j = 0; j < 2; j++) {
							spawnabaddy(i, GRONSW);
						}
						break;
					case 3:// SPAWN SKELETONS
						for (j = 0; j < 4; j++) {
							spawnabaddy(i, SKELETON);
						}
						break;
					case 4:
						castspell(plr, i);
						break;
					}
				}
			}
		}
		newstatus(i, CHASE);
	}
	else if (spr.picnum == JUDYATTACK2 + 8) {
		sprite[i].picnum = JUDYATTACK2;
		playsound_loc(S_JUDY1 + krand() % 4, sprite[i].x, sprite[i].y);
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
					spawnabaddy(i, WILLOW);
					break;
				case 1:// SPAWN 6 SPIDERS
					for (j = 0; j < 4; j++) {
						spawnabaddy(i, SPIDER);
					}
					break;
				case 2:// SPAWN 2 GRONSW
					for (j = 0; j < 2; j++) {
						spawnabaddy(i, GRONSW);
					}
					break;
				case 3:// SPAWN SKELETONS
					for (j = 0; j < 4; j++) {
						spawnabaddy(i, SKELETON);
					}
					break;
				case 4:
					castspell(plr, i);
					break;
				}
			}

		}
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

		if (spr.picnum == JUDYDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
		}
	}
}
	
void judyOperate(PLAYER& plr)
{
	short nextsprite;
	for (short i = headspritestat[WITCHSIT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].ang = (short)(getangle(plr.x - sprite[i].x, plr.y - sprite[i].y) & 2047);
		if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
			sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum)) {
			sprite[i].lotag -= TICSPERFRAME;
			if (sprite[i].lotag < 0) {
				sprite[i].picnum++;
				sprite[i].lotag = 12;
				if (sprite[i].picnum == JUDYSIT + 4) {
					sprite[i].picnum = JUDY;
					newstatus(i, FACE);
				}
			}
		}
	}
}
	
void spawnabaddy(int i, int monster) {
	short j = insertsprite(sprite[i].sectnum, FACE);

	sprite[j].x = sprite[i].x + (krand() & 2048) - 1024;
	sprite[j].y = sprite[i].y + (krand() & 2048) - 1024;
	sprite[j].z = sprite[i].z;

	sprite[j].pal = 0;
	sprite[j].shade = 0;
	sprite[j].cstat = 0;

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

	sprite[j].picnum = (short)monster;
	killcnt++;

	setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
}


void createJudyAI() {
	auto& e = enemy[JUDYTYPE];
	e.info.Init(32, 32, 2048, 120, 0, 64, false, 500, 0);
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
