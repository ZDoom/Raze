#include "ns.h"
#include "wh.h"

BEGIN_WH_NS


static void stand(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];

	if (sintable[(spr.ang + 512) & 2047] * (plr.x - spr.x)
		+ sintable[spr.ang & 2047] * (plr.y - spr.y) >= 0)
		if (cansee(spr.x, spr.y, spr.z - (tilesizy[spr.picnum] << 7), spr.sectnum, plr.x, plr.y,
			plr.z, plr.sector) && plr.invisibletime < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
				newstatus(i, FLEE);
			}
			else
				newstatus(i, CHASE);
		}
}

static void nuked(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(i, false);
	newstatus(i, DIE);
}

static void kurtExplo(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	spr.picnum++;
	if (spr.lotag < 0)
		spr.lotag = 12;

	short j = headspritesect[spr.sectnum];
	while (j != -1) {
		short nextj = nextspritesect[j];
		long dx = klabs(spr.x - sprite[j].x); // x distance to sprite
		long dy = klabs(spr.y - sprite[j].y); // y distance to sprite
		long dz = klabs((spr.z >> 8) - (sprite[j].z >> 8)); // z distance to sprite
		long dh = tilesizy[sprite[j].picnum] >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (sprite[j].detail == KURTTYPE) {
				sprite[j].hitag -= TICSPERFRAME << 4;
				if (sprite[j].hitag < 0) {
					newstatus(j, DIE);
				}
			}
		}
		j = nextj;
	}
}

void createKurtAI() {
	auto& e = enemy[KURTTYPE];
	e.info.Init(35, 35, 1024 + 256, 120, 0, 48, false, 50, 0);
	e.info.getAttackDist = [](EnemyInfo& e, SPRITE& spr) {
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
	e.stand = stand;
	e.nuked = nuked;
	e.chase = enemy[GONZOTYPE].chase;
	e.resurect = enemy[GONZOTYPE].resurect;
	e.skirmish = enemy[GONZOTYPE].skirmish;
	e.search = enemy[GONZOTYPE].search;
	e.frozen = enemy[GONZOTYPE].frozen;
	e.pain = enemy[GONZOTYPE].pain;
	e.face = enemy[GONZOTYPE].face;
	e.attack = enemy[GONZOTYPE].attack;
	e.cast = enemy[GONZOTYPE].cast;
	e.flee = enemy[GONZOTYPE].flee;
	e.die = enemy[GONZOTYPE].die;
}

void premapKurt(short i) {
	SPRITE& spr = sprite[i];
	spr.detail = KURTTYPE;
	enemy[KURTTYPE].info.set(spr);
	changespritestat(i, STAND);

	switch (spr.picnum) {
	case KURTSTAND:
		spr.extra = 20;
		break;
	case KURTKNEE:
		spr.extra = 10;
		break;
	}
}

END_WH_NS
