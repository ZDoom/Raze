#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void gonzopike(DWHActor* s, PLAYER& plr);
static void checkexplgonzo(PLAYER& plr, DWHActor* actor);
static boolean patrolprocess(PLAYER& plr, DWHActor* actor);

static void chasegonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;

	switch (spr.picnum) {
	case GONZOGHM:
	case GONZOGSH:
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			if (checkdist(plr, actor)) {
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
				}
				else {
					SetNewStatus(actor, ATTACK);
				}
				break;
			}
			else if ((krand() & 0) == 1) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			if (krand() % 63 > 60) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
				break;
			}

			int dax = spr.x; // Back up old x&y if stepping off cliff
			int day = spr.y;
			int daz = spr.z;

			osectnum = spr.sectnum;
			auto moveStat = aimove(actor);
			if (moveStat.type == kHitFloor)
			{
				spr.ang = (short)((spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
				return;
			}

			if (zr_florz > spr.z + (48 << 8)) {
				spr.x = dax;
				spr.y = day;
				spr.z = daz;
				SetActorPos(actor, &spr.pos);
				moveStat.type = -1; // make invalid.

				if (rand() % 100 > 80 && plr.Sector()->lotag == 25) {
					SetNewStatus(actor, AMBUSH);
					spr.z -= (getPlayerHeight() << 6);
					spr.lotag = 60;
					spr.extra = 1;
					spr.picnum = GONZOHMJUMP;
					return;

				}

			}

			if (moveStat.type == kHitWall && plr.Sector()->lotag == 25) {
				SetNewStatus(actor, AMBUSH);
				spr.z -= (getPlayerHeight() << 6);
				spr.lotag = 90;
				spr.extra = 3;
				spr.picnum = GONZOHMJUMP;
				return;
			}

			if (moveStat.type != kHitNone) {
				if (moveStat.type == kHitSprite && moveStat.actor == plr.actor()) {
					int daang;
					if ((krand() & 0) == 1)
						daang = (spr.ang + 256) & 2047;
					else
						daang = (spr.ang - 256) & 2047;
					spr.ang = (short)daang;
					if (plr.shadowtime > 0) {
						spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						SetNewStatus(actor, FLEE);
					}
					else {
						SetNewStatus(actor, SKIRMISH);
					}
				}
				else {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					SetNewStatus(actor, SKIRMISH);
				}
			}
			break;
		}
		else {
			if (!patrolprocess(plr, actor))
				SetNewStatus(actor, FLEE);
		}
		break;
	case GONZOCSW:
	case GONZOGSW:
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && plr.invisibletime < 0) {
			if (checkdist(plr, actor)) {
				if (plr.shadowtime > 0) {
					spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
				}
				else
					SetNewStatus(actor, ATTACK);
			}
			else if (krand() % 63 > 60) {
				spr.ang = (short)(((krand() & 128 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			else {
				auto moveStat = aimove(actor);
				if (moveStat.type == kHitFloor)
				{
					spr.ang = (short)((spr.ang + 1024) & 2047);
					SetNewStatus(actor, FLEE);
					return;
				}

				if (moveStat.type == kHitSprite) {
					if (moveStat.actor != plr.actor()) {
						short daang = (short)((spr.ang - 256) & 2047);
						spr.ang = daang;
						if (plr.shadowtime > 0) {
							spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
							SetNewStatus(actor, FLEE);
						}
						else
							SetNewStatus(actor, SKIRMISH);
					}
					else {
						spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						SetNewStatus(actor, SKIRMISH);
					}
				}
			}
		}
		else {
			if (!patrolprocess(plr, actor))
				SetNewStatus(actor, FLEE);
		}

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
		spr.z = zr_florz;

		if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
			warpsprite(actor);

		if (checksector6(actor))
			return;

		processfluid(actor, zr_florHit, false);

		if (sector[osectnum].lotag == KILLSECTOR) {
			spr.hitag--;
			if (spr.hitag < 0)
				SetNewStatus(actor, DIE);
		}

		SetActorPos(actor, &spr.pos);

		if (zr_florHit.type == kHitSector && (spr.sector()->floorpicnum == LAVA
			|| spr.sector()->floorpicnum == LAVA1 || spr.sector()->floorpicnum == ANILAVA)) {
			spr.hitag--;
			if (spr.hitag < 0)
				SetNewStatus(actor, DIE);
		}
	}

	checkexplgonzo(plr, actor);
}
	
static void resurectgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
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
	
static void skirmishgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag < 0)
		SetNewStatus(actor, FACE);
	short osectnum = spr.sectnum;

	auto moveStat = aimove(actor);
	if (moveStat.type != kHitFloor && moveStat.type != kHitNone) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		SetNewStatus(actor, FACE);
	}
	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	processfluid(actor, zr_florHit, false);

	SetActorPos(actor, &spr.pos);

	if (checksector6(actor))
		return;

	checkexplgonzo(plr, actor);
}

static void searchgonzo(PLAYER& plr, DWHActor* actor)
{
	if (!checksector6(actor))
		checkexplgonzo(plr, actor);
}
	
static void nukedgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	chunksofmeat(plr, actor,spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(actor,false);
	SetNewStatus(actor, DIE);
}
	
static void frozengonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.pal = 0;
		switch (spr.picnum) {
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
		SetNewStatus(actor, FACE);
	}
}
	
static void paingonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		switch (spr.picnum) {
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
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);

	checkexplgonzo(plr, actor);
}
	
static void facegonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = (short)(getangle(plr.x - spr.x, plr.y - spr.y) & 2047);

		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
		else {
			actor->SetOwner(plr.actor());
			SetNewStatus(actor, CHASE);
		}
	}
	else { // get off the wall
		if (actor->GetOwner() == plr.actor()) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang) & 2047);
			SetNewStatus(actor, FINDME);
		}
		else if (cansee) SetNewStatus(actor, FLEE);
	}

	if (plr.invisibletime < 0 && checkdist(plr, actor))
		SetNewStatus(actor, ATTACK);

	checkexplgonzo(plr, actor);
}
	
static void attackgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.z = zr_florz;

	switch (checkfluid(actor, zr_florHit)) {
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
			if (checksight(plr, actor))
				if (checkdist(plr, actor)) {
					spr.ang = (short)checksight_ang;
					attack(plr, actor);
				}
		}
		else if (spr.lotag < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
		spr.lotag -= TICSPERFRAME;
		break;
	case GONZOCSWAT:
		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
				spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum))
				SetNewStatus(actor, CAST);
			else
				SetNewStatus(actor, CHASE);
		}
		else
			spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		break;
	case GONZOGSWAT:
	case GONZOGHMAT:
	case GONZOGSHAT:
		if (spr.lotag == 31) {
			if (checksight(plr, actor))
				if (checkdist(plr, actor)) {
					spr.ang = (short)checksight_ang;
					attack(plr, actor);
				}
		}
		else if (spr.lotag < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
		spr.lotag -= TICSPERFRAME;
		break;
	}

	checksector6(actor);
}
	
static void fleegonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	auto moveStat = aimove(actor);

	if (moveStat.type != kHitFloor && moveStat.type != kHitNone) {
		if (moveStat.type == kHitWall) {
			int nWall = moveStat.index;
			int nx = -(wall[wall[nWall].point2].y - wall[nWall].y) >> 4;
			int ny = (wall[wall[nWall].point2].x - wall[nWall].x) >> 4;
			spr.ang = getangle(nx, ny);
		}
		else {
			if (plr.invisibletime < 0) {
				spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
				SetNewStatus(actor, FACE);
			}
		}
	}
	if (spr.lotag < 0)
		SetNewStatus(actor, FACE);

	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	if (checksector6(actor))
		return;

	processfluid(actor, zr_florHit, false);

	SetActorPos(actor, &spr.pos);

	checkexplgonzo(plr, actor);
}
	
static void castgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == GONZOCSWAT) {
		spr.extra--;
		spritesound(S_GENTHROW, actor);
		gonzopike(actor, plr);
		SetNewStatus(actor, CHASE);
	}
}
	
static void diegonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		switch (spr.picnum) {
		case GONZOBSHDEAD:
		case GONZOCSWDEAD:
		case GONZOGSWDEAD:
		case GONZOGHMDEAD:
		case GONZOGSHDEAD:
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
			break;
		}
	}
}

void gonzoProcess(PLAYER& plr)
{
	WHStatIterator it(LAND);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

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
			spr.hitag = adjusthp(100);
			SetNewStatus(actor, FACE);
			break;
		case GONZOSHJUMPEND:
			spr.picnum = GONZOGSH;
			spr.detail = GONZOTYPE;
			enemy[GONZOTYPE].info.set(spr);
			spr.hitag = adjusthp(100);
			SetNewStatus(actor, FACE);
			break;
		}
	}

	Collision moveStat;
	it.Reset(AMBUSH);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		switch (spr.extra) {
		case 1: // forward
			spr.zvel += TICSPERFRAME << 3;

			moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			if (zr_florz <= spr.z && spr.lotag < 0) {
				spr.z = zr_florz;
				ChangeActorStat(actor, LAND);
			}

			if (moveStat.type== kHitSprite) { // Bullet hit a sprite
				int k = moveStat.actor->GetSpriteIndex();
				for (int j = 0; j < 15; j++) {
					shards(k, 1);
				}
				damageactor(plr, moveStat.actor, actor);
			}

			break;
		case 2: // fall
			spr.zvel += TICSPERFRAME << 4;

			moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 1,
				(bsin(spr.ang) * TICSPERFRAME) << 1, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			if (zr_florz <= spr.z && spr.lotag < 0) {
				spr.z = zr_florz;
				ChangeActorStat(actor, LAND);
			}

			break;
		case 3: // jumpup

			spr.zvel -= TICSPERFRAME << 4;

			moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);

			spr.lotag -= TICSPERFRAME;

			SetActorPos(actor, &spr.pos);

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

	WHStatIterator it(ATTACK2);
	while (auto itActor = it.Next())
	{
		SPRITE& tspr = itActor->s();
		int j = itActor->GetSpriteIndex();

		int dist = abs(tspr.x - spr.x) + abs(tspr.y - spr.y);
		if (dist < mindist) {
			mindist = dist;
			target = j;
		}
	}

	return target;
}
	
static boolean patrolprocess(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	short target = searchpatrol(spr);
	if (target != -1) {
		SPRITE& tspr = sprite[target];
		if (cansee(tspr.x, tspr.y, tspr.z, tspr.sectnum, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum)) {
			spr.ang = getangle(tspr.x - spr.x, tspr.y - spr.y);
			SetNewStatus(actor, PATROL);
		}
	}

	return target != -1;
}

static void gonzopike(DWHActor* actor, PLAYER& plr) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, JAVLIN);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z - (40 << 8);

	spawned.cstat = 21;
	spawned.picnum = THROWPIKE;
	spawned.ang = (short)(((spr.ang + 2048 + 96) - 512) & 2047);
	spawned.xrepeat = 24;
	spawned.yrepeat = 24;
	spawned.clipdist = 32;

	spawned.extra = spr.ang;
	spawned.shade = -15;
	spawned.xvel = (short)((krand() & 256) - 128);
	spawned.yvel = (short)((krand() & 256) - 128);

	spawned.zvel = (short)(((plr.z + (8 << 8) - spr.z) << 7) / ksqrt((plr.x - spr.x) * (plr.x - spr.x) + (plr.y - spr.y) * (plr.y - spr.y)));

	spawned.zvel += ((krand() % 256) - 128);

	spawnedactor->SetOwner(actor);
	spawned.lotag = 1024;
	spawned.hitag = 0;
	spawned.pal = 0;
	spr.backuploc();

}

static void checkexplgonzo(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	WHSectIterator it(spr.sectnum);
	while (auto sectactor = it.Next())
	{
		SPRITE& tspr = sectactor->s();
		int j = sectactor->GetSpriteIndex();

		int dx = abs(spr.x - tspr.x); // x distance to sprite
		int dy = abs(spr.y - tspr.y); // y distance to sprite
		int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
		int dh = tileHeight(tspr.picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if (tspr.picnum == EXPLO2
				|| tspr.picnum == MONSTERBALL) {
				spr.hitag -= TICSPERFRAME << 2;
				if (spr.hitag < 0) {
					SetNewStatus(actor, DIE);
				}
			}
		}
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
	

void premapGonzo(DWHActor* actor) {
	SPRITE& spr = actor->s();

	spr.detail = GONZOTYPE;
	enemy[GONZOTYPE].info.set(spr);
	ChangeActorStat(actor, FACE);

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
	auto& spr = sprite[sn];
	auto spawnedactor = InsertActor(spr.sectnum, DEAD);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z;
	spawned.cstat = 0;
	spawned.picnum = GONZOBSHDEAD;
	spawned.shade = spr.sector()->floorshade;
	spawned.pal = 0;
	spawned.xrepeat = spr.xrepeat;
	spawned.yrepeat = spr.yrepeat;
	spawned.owner = 0;
	spawned.lotag = 0;
	spawned.hitag = 0;
	spawned.backuploc();
}

END_WH_NS
