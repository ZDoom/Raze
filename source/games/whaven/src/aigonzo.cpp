#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void gonzopike(short s, PLAYER& plr);
static void checkexplgonzo(PLAYER& plr, short i);
static boolean patrolprocess(PLAYER& plr, short i);

static void chasegonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	switch (spr.picnum) {
	case GONZOGHM:
	case GONZOGSH:
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			if (checkdist(plr, i)) {
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					newstatus(i, FLEE);
				}
				else {
					newstatus(i, ATTACK);
				}
				break;
			}
			else if ((krand() & 0) == 1) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
			}
			if (krand() % 63 > 60) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
				break;
			}

			int dax = spr.x; // Back up old x&y if stepping off cliff
			int day = spr.y;
			int daz = spr.z;

			osectnum = spr.sectnum;
			int movestat = aimove(i);
			if ((movestat & kHitTypeMask) == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
				return;
			}

			if (zr_florz > spr.z + (48 << 8)) {
				spr.x = dax;
				spr.y = day;
				spr.z = daz;
				setsprite(i, spr.x, spr.y, spr.z);
				movestat = 1;

				if (rand() % 100 > 80 && sector[plr.sector].lotag == 25) {
					newstatus(i, AMBUSH);
					sprite[i].z -= (getPlayerHeight() << 6);
					sprite[i].lotag = 60;
					sprite[i].extra = 1;
					sprite[i].picnum = GONZOHMJUMP;
					return;

				}

			}

			if ((movestat & 0xc000) == 32768 && sector[plr.sector].lotag == 25) {
				newstatus(i, AMBUSH);
				sprite[i].z -= (getPlayerHeight() << 6);
				sprite[i].lotag = 90;
				sprite[i].extra = 3;
				sprite[i].picnum = GONZOHMJUMP;
				return;
			}

			if (movestat != 0) {
				if ((movestat & 4095) != plr.spritenum) {
					int daang;
					if ((krand() & 0) == 1)
						daang = (spr.ang + 256) & 2047;
					else
						daang = (spr.ang - 256) & 2047;
					spr.ang = (short)daang;
					if (plr.shadowtime > 0) {
						spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						newstatus(i, FLEE);
					}
					else {
						newstatus(i, SKIRMISH);
					}
				}
				else {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					newstatus(i, SKIRMISH);
				}
			}
			break;
		}
		else {
			if (!patrolprocess(plr, i))
				newstatus(i, FLEE);
		}
		break;
	case GONZOCSW:
	case GONZOGSW:
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
			if (!patrolprocess(plr, i))
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
	}

	checkexplgonzo(plr, i);
}
	
static void resurectgonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		newstatus(i, FACE);
		switch (spr.picnum) {
		case GONZOCSWDEAD:
			spr.picnum = GONZOCSW;
			spr.hitag = (short)adjusthp(50);
			break;
		case GONZOGSWDEAD:
			spr.picnum = GONZOGSW;
			spr.hitag = (short)adjusthp(100);
			break;
		case GONZOGHMDEAD:
			spr.picnum = GONZOGHM;
			spr.hitag = (short)adjusthp(40);
			break;
		case GONZOGSHDEAD:
			spr.picnum = GONZOGSH;
			spr.hitag = (short)adjusthp(50);
			break;
		}
		spr.lotag = 100;
		spr.cstat |= 1;
	}
}
	
static void skirmishgonzo(PLAYER& plr, short i) {
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

	checkexplgonzo(plr, i);
}

static void searchgonzo(PLAYER& plr, short i) {
	aisearch(plr, i, false);
	if (!checksector6(i))
		checkexplgonzo(plr, i);
}
	
static void nukedgonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(i, false);
	newstatus(i, DIE);
}
	
static void frozengonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		switch (sprite[i].picnum) {
		case GONZOCSWPAIN:
			spr.picnum = GONZOCSW;
			break;
		case GONZOGSWPAIN:
			spr.picnum = GONZOGSW;
			break;
		case GONZOGHMPAIN:
			spr.picnum = GONZOGHM;
			break;
		case GONZOGSHPAIN:
			spr.picnum = GONZOGSH;
			break;
		}
		newstatus(i, FACE);
	}
}
	
static void paingonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		switch (sprite[i].picnum) {
		case GONZOCSWPAIN:
			spr.picnum = GONZOCSW;
			break;
		case GONZOGSWPAIN:
			spr.picnum = GONZOGSW;
			break;
		case GONZOGHMPAIN:
			spr.picnum = GONZOGHM;
			break;
		case GONZOGSHPAIN:
			spr.picnum = GONZOGSH;
			break;
		}
		spr.ang = plr.angle.ang.asbuild();
		newstatus(i, FLEE);
	}

	aimove(i);
	processfluid(i, zr_florhit, false);
	setsprite(i, spr.x, spr.y, spr.z);

	checkexplgonzo(plr, i);
}
	
static void facegonzo(PLAYER& plr, short i) {
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

	if (plr.invisibletime < 0 && checkdist(plr, i))
		newstatus(i, ATTACK);

	checkexplgonzo(plr, i);
}
	
static void attackgonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
	case TYPEWATER:
		spr.z += tileHeight(spr.picnum) << 5;
		break;
	}

	switch (spr.picnum) {
		// WANGO
	case KURTREADY:
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum++;
			spr.lotag = 24;
		}
		break;
	case KURTREADY + 1:
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum = KURTAT;
			spr.lotag = 64;
		}
		break;
	case KURTAT:
	case KURTPUNCH:
		if (spr.lotag == 46) {
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
		break;
	case GONZOCSWAT:
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
		break;
	case GONZOGSWAT:
	case GONZOGHMAT:
	case GONZOGSHAT:
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
		break;
	}

	checksector6(i);
}
	
static void fleegonzo(PLAYER& plr, short i) {
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
			if (plr.invisibletime < 0) {
				spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
				newstatus(i, FACE);
			}
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

	checkexplgonzo(plr, i);
}
	
static void castgonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == GONZOCSWAT) {
		spr.extra--;
		spritesound(S_GENTHROW, &spr);
		gonzopike(i, plr);
		newstatus(i, CHASE);
	}
}
	
static void diegonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		switch (sprite[i].picnum) {
		case GONZOBSHDEAD:
		case GONZOCSWDEAD:
		case GONZOGSWDEAD:
		case GONZOGHMDEAD:
		case GONZOGSHDEAD:
			if (difficulty == 4)
				newstatus(i, RESURECT);
			else {
				kills++;
				newstatus(i, DEAD);
			}
			break;
		}
	}
}

void gonzoProcess(PLAYER& plr)
{
	short nextsprite;
	for (short i = headspritestat[LAND]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		SPRITE& spr = sprite[i];
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.lotag = 12;
			spr.picnum++;
		}

		switch (spr.picnum) {
		case GONZOHMJUMPEND:
			spr.picnum = GONZOGSH;
			spr.detail = GONZOTYPE;
			enemy[GONZOTYPE].info.set(spr);
			sprite[i].hitag = adjusthp(100);
			newstatus(i, FACE);
			break;
		case GONZOSHJUMPEND:
			spr.picnum = GONZOGSH;
			spr.detail = GONZOTYPE;
			enemy[GONZOTYPE].info.set(spr);
			sprite[i].hitag = adjusthp(100);
			newstatus(i, FACE);
			break;
		}
	}

	short movestat;
	for (short i = headspritestat[AMBUSH]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		SPRITE& spr = sprite[i];
		switch (spr.extra) {
		case 1: // forward
			spr.zvel += TICSPERFRAME << 3;

			movestat = (short)movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			if (zr_florz <= spr.z && spr.lotag < 0) {
				spr.z = zr_florz;
				changespritestat(i, LAND);
			}

			if ((movestat & 0xc000) == 49152) { // Bullet hit a sprite
				int k = (movestat & 4095);
				for (int j = 0; j < 15; j++) {
					shards(k, 1);
				}
				damageactor(plr, movestat, i);
			}

			break;
		case 2: // fall
			spr.zvel += TICSPERFRAME << 4;

			movestat = (short)movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 1,
				(bsin(spr.ang) * TICSPERFRAME) << 1, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			if (zr_florz <= spr.z && spr.lotag < 0) {
				spr.z = zr_florz;
				changespritestat(i, LAND);
			}

			break;
		case 3: // jumpup

			spr.zvel -= TICSPERFRAME << 4;

			movestat = (short)movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			setsprite(i, spr.x, spr.y, spr.z);

			if (spr.lotag < 0) {
				spr.extra = 2;
				spr.lotag = 20;
			}

			break;
		}
	}
}

static short searchpatrol(SPRITE& spr) {
	int mindist = 0x7fffffff;
	short target = -1;
	short j = headspritestat[APATROLPOINT];
	while (j != -1) {
		short nextj = nextspritestat[j];
		SPRITE& tspr = sprite[j];
		int dist = abs(tspr.x - spr.x) + abs(tspr.y - spr.y);
		if (dist < mindist) {
			mindist = dist;
			target = j;
		}
		j = nextj;
	}

	return target;
}
	
static boolean patrolprocess(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	short target = searchpatrol(spr);
	if (target != -1) {
		SPRITE& tspr = sprite[target];
		if (cansee(tspr.x, tspr.y, tspr.z, tspr.sectnum, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum)) {
			spr.ang = getangle(tspr.x - spr.x, tspr.y - spr.y);
			newstatus(i, PATROL);
		}
	}

	return target != -1;
}

static void gonzopike(short s, PLAYER& plr) {
	int j = insertsprite(sprite[s].sectnum, JAVLIN);
	if (j == -1)
		return;

	SPRITE& spr = sprite[j];

	spr.x = sprite[s].x;
	spr.y = sprite[s].y;
	spr.z = sprite[s].z - (40 << 8);

	spr.cstat = 21;
	spr.picnum = THROWPIKE;
	spr.ang = (short)(((sprite[s].ang + 2048 + 96) - 512) & 2047);
	spr.xrepeat = 24;
	spr.yrepeat = 24;
	spr.clipdist = 32;

	spr.extra = sprite[s].ang;
	spr.shade = -15;
	spr.xvel = (short)((krand() & 256) - 128);
	spr.yvel = (short)((krand() & 256) - 128);

	spr.zvel = (short)(((plr.z + (8 << 8) - sprite[s].z) << 7) / ksqrt((plr.x - sprite[s].x) * (plr.x - sprite[s].x) + (plr.y - sprite[s].y) * (plr.y - sprite[s].y)));

	spr.zvel += ((krand() % 256) - 128);

	spr.owner = s;
	spr.lotag = 1024;
	spr.hitag = 0;
	spr.pal = 0;

}

static void checkexplgonzo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	short j = headspritesect[spr.sectnum];
	while (j != -1) {
		short nextj = nextspritesect[j];
		long dx = abs(spr.x - sprite[j].x); // x distance to sprite
		long dy = abs(spr.y - sprite[j].y); // y distance to sprite
		long dz = abs((spr.z >> 8) - (sprite[j].z >> 8)); // z distance to sprite
		long dh = tileHeight(sprite[j].picnum) >> 1; // height of sprite
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

void createGonzoAI() {
	auto &e = enemy[GONZOTYPE];
	e.info.Init(35, 35, 1024 + 256, 120, 0, 48, false, 50, 0);
	e.info.getAttackDist = [](EnemyInfo& e, SPRITE& spr)
	{
		int out = e.attackdist;
		switch (spr.picnum) {
		case KURTAT:
		case GONZOCSW:
		case GONZOCSWAT:
			if (spr.extra > 10)
				out = 2048 << 1;
			break;
		}

		return out;
	};

	e.info.getHealth = [](EnemyInfo& e, SPRITE& spr) ->short
	{
		switch (spr.picnum) {
		case KURTAT:
			return 10;
		case KURTPUNCH:
			return adjusthp(15);
		case GONZOGSW:
			return adjusthp(100);
		case GONZOGHM:
			return adjusthp(40);
		}

		return adjusthp(e.health);
	};
	e.chase = chasegonzo;
	e.resurect = resurectgonzo;
	e.skirmish = skirmishgonzo;
	e.search = searchgonzo;
	e.nuked = nukedgonzo;
	e.frozen = frozengonzo;
	e.pain = paingonzo;
	e.face = facegonzo;
	e.attack = attackgonzo;
	e.flee = fleegonzo;
	e.cast = castgonzo;
	e.die = diegonzo;
}
	

void premapGonzo(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = GONZOTYPE;
	enemy[GONZOTYPE].info.set(spr);
	changespritestat(i, FACE);

	switch (spr.picnum) {
	case KURTAT:
		spr.picnum = GONZOCSW;
		break;
	case KURTPUNCH:
		spr.extra = 0;
		spr.picnum = GONZOCSW;
		break;
	case GONZOCSW:
		spr.extra = 20;
		break;
	case GONZOGSW:
	case GONZOGHM:
	case GONZOGSH:
		spr.clipdist = 32;
		spr.extra = 0;
		break;
	}
}
	
void deaddude(short sn) {
	int j = insertsprite(sprite[sn].sectnum, DEAD);
	sprite[j].x = sprite[sn].x;
	sprite[j].y = sprite[sn].y;
	sprite[j].z = sprite[sn].z;
	sprite[j].cstat = 0;
	sprite[j].picnum = GONZOBSHDEAD;
	sprite[j].shade = sector[sprite[sn].sectnum].floorshade;
	sprite[j].pal = 0;
	sprite[j].xrepeat = sprite[sn].xrepeat;
	sprite[j].yrepeat = sprite[sn].yrepeat;
	sprite[j].owner = 0;
	sprite[j].lotag = 0;
	sprite[j].hitag = 0;
}

END_WH_NS
