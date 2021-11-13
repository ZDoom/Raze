#include "ns.h"
#include "wh.h"

BEGIN_WH_NS


static void standkurt(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
		if (cansee(spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum, plr.x, plr.y,
			plr.z, plr.sector) && plr.invisibletime < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
	}
}

static void nukedkurt(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	chunksofmeat(plr, i, spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(i, false);
	SetNewStatus(actor, DIE);
}

static void kurtExplo(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	spr.picnum++;
	if (spr.lotag < 0)
		spr.lotag = 12;

	WHSectIterator it(spr.sectnum);
	while (auto sect = it.Next())
	{
		SPRITE& tspr = sect->s();
		int j = sect->GetSpriteIndex();

		int dx = abs(spr.x - tspr.x); // x distance to sprite
		int dy = abs(spr.y - tspr.y); // y distance to sprite
		int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
		int dh = tileHeight(tspr.picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (tspr.detail == KURTTYPE) {
				tspr.hitag -= TICSPERFRAME << 4;
				if (tspr.hitag < 0) {
					newstatus(j, DIE);
				}
			}
		}
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
	e.stand = standkurt;
	e.nuked = nukedkurt;
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
