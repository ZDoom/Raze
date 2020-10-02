#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void checkexpl(PLAYER& plr, short i);
static void throwhalberd(int s);


static void chase(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	if (spr.picnum == GRONSW) {
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
	}
	else {
		if (krand() % 63 == 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
				sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum))// && invisibletime < 0)
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
		switch (krand() % 3) {
		case 0:
			sprite[i].picnum = GRONHAL;
			sprite[i].hitag = (short)adjusthp(120);
			sprite[i].extra = 3;
			break;
		case 1:
			sprite[i].picnum = GRONSW;
			sprite[i].hitag = (short)adjusthp(120);
			sprite[i].extra = 0;
			break;
		case 2:
			sprite[i].picnum = GRONMU;
			sprite[i].hitag = (short)adjusthp(120);
			sprite[i].extra = 2;
			break;
		}
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
	
static void search(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	if (!checksector6(i))
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
		if (spr.picnum == GRONCHAR + 4) {
			trailingsmoke(i, false);
			deletesprite(i);
		}
	}
}
	
static void frozen(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		if (sprite[i].picnum == GRONHALDIE)
			sprite[i].picnum = GRONHAL;
		else if (sprite[i].picnum == GRONSWDIE)
			sprite[i].picnum = GRONSW;
		else if (sprite[i].picnum == GRONMUDIE)
			sprite[i].picnum = GRONMU;
		newstatus(i, FACE);
	}
}
			
static void pain(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (sprite[i].picnum == GRONHALPAIN)
			sprite[i].picnum = GRONHAL;
		else if (sprite[i].picnum == GRONSWPAIN)
			sprite[i].picnum = GRONSW;
		else if (sprite[i].picnum == GRONMUPAIN)
			sprite[i].picnum = GRONMU;

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

	if (spr.picnum == GRONSWATTACK) {
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
	}
	else {
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
				spr.z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum))
				newstatus(i, CAST);
			else
				newstatus(i, CHASE);
		}
		else
			spr.ang = getangle(plr.x - sprite[i].x, plr.y - sprite[i].y);
	}

	checksector6(i);
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
	
static void cast(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		if (spr.picnum == GRONHALATTACK) {
			spr.extra--;
			playsound_loc(S_THROWPIKE, sprite[i].x, sprite[i].y);
			throwhalberd(i);
			newstatus(i, CHASE);
		}
		else if (spr.picnum == GRONMUATTACK) {
			spr.extra--;
			playsound_loc(S_SPELL2, sprite[i].x, sprite[i].y);
			castspell(plr, i);
			newstatus(i, CHASE);
		}
	}

	checksector6(i);
}
	
static void die(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.picnum == GRONSWDIE || spr.picnum == GRONHALDIE || spr.picnum == GRONMUDIE)
	{
		if (spr.lotag < 0) {
			spr.picnum = GRONDIE;
			spr.lotag = 20;
		}
		else
			return;
	}

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == GRONDEAD) {
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
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

static void throwhalberd(int s) {
	int j = insertsprite(sprite[s].sectnum, JAVLIN);

	if (j == -1)
		return;
	SPRITE& spr = sprite[j];
	spr.x = sprite[s].x;
	spr.y = sprite[s].y;
	spr.z = sprite[s].z - (40 << 8);

	spr.cstat = 17;

	spr.picnum = THROWHALBERD;
	spr.detail = THROWHALBERDTYPE;
	spr.ang = (short)(((sprite[s].ang + 2048) - 512) & 2047);
	spr.xrepeat = 8;
	spr.yrepeat = 16;
	spr.clipdist = 32;

	spr.extra = sprite[s].ang;
	spr.shade = -15;
	spr.xvel = (short)((krand() & 256) - 128);
	spr.yvel = (short)((krand() & 256) - 128);
	spr.zvel = (short)((krand() & 256) - 128);
	spr.owner = (short)s;
	spr.lotag = 0;
	spr.hitag = 0;
	spr.pal = 0;

	spr.cstat = 0;
	int daz = (((spr.zvel) * TICSPERFRAME) >> 3);
	movesprite((short)j, ((sintable[(spr.extra + 512) & 2047]) * TICSPERFRAME) << 7,
		((sintable[spr.extra & 2047]) * TICSPERFRAME) << 7, daz, 4 << 8, 4 << 8, 1);
	spr.cstat = 21;
}

void createGronAI() {
	auto& e = enemy[GRONTYPE];
	e.info.Init(isWh2() ? 35 : 30, isWh2() ? 35 : 30, -1, 120, 0, 64, false, 300, 0);
	e.info.getAttackDist = [](EnemyInfo& e, SPRITE& spr)
	{
		int out = e.attackdist;
		int pic = spr.picnum;

		if (pic == GRONHAL || pic == GRONHALATTACK)
			out = 1024 + 512;
		else if (pic == GRONMU || pic == GRONMUATTACK)
			out = 2048;
		else  if (pic == GRONSW || pic == GRONSWATTACK)
			out = 1024 + 256;

		return out;
	};

	e.info.getHealth = [](EnemyInfo& e, SPRITE& spr)
	{
		if (isWh2()) {
			if (spr.picnum == GRONHAL)
				return adjusthp(65);
			if (spr.picnum == GRONMU)
				return adjusthp(70);
		}
		return adjusthp(e.health);
	};
	e.chase = chase;
	e.resurect = resurect;
	e.skirmish = skirmish;
	e.search = search;
	e.nuked = nuked;
	e.frozen = frozen;
	e.pain = pain;
	e.face = face;
	e.attack = attack;
	e.flee = flee;
	e.cast = cast;
	e.die = die;
}


void premapGron(short i) {
	SPRITE& spr = sprite[i];

	if (spr.picnum == GRONSW && spr.pal == 10)
		deletesprite(i);

	spr.detail = GRONTYPE;
	enemy[GRONTYPE].info.set(spr);
	changespritestat(i, FACE);

	if (spr.picnum == GRONHAL)
		spr.extra = 4;
	else if (spr.picnum == GRONSW)
		spr.extra = 0;
	else if (spr.picnum == GRONMU)
		spr.extra = 2;
}

END_WH_NS
