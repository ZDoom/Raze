#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void newguyarrow(DWHActor* s, PLAYER& plr);

static void standnewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
		if (cansee(spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum, plr.x, plr.y,
			plr.z, plr.sector) && plr.invisibletime < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
	}
}
	
static void chasenewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0)
		spr.lotag = 250;

	short osectnum = spr.sectnum;
	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && plr.invisibletime < 0) {
		if (checkdist(plr, actor)) {
			if (plr.shadowtime > 0) {
				spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, ATTACK);
		}
		else if (krand() % 63 > 60) {
			spr.ang = (((krand() & 128 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
		else {
			auto moveStat = aimove(actor);
			if (moveStat.type == kHitFloor)
			{
				spr.ang = ((spr.ang + 1024) & 2047);
				SetNewStatus(actor, FLEE);
				return;
			}

			if (moveStat.type == kHitSprite) {
				if (moveStat.actor != plr.actor()) {
					short daang = ((spr.ang - 256) & 2047);
					spr.ang = daang;
					if (plr.shadowtime > 0) {
						spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
						SetNewStatus(actor, FLEE);
					}
					else
						SetNewStatus(actor, SKIRMISH);
				}
				else {
					spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
					SetNewStatus(actor, SKIRMISH);
				}
			}
		}
	}
	else {
		spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
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
	
static void resurectnewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		int j = krand() % 3;
		switch (j) {
		case 0:
			spr.extra = 30;
			spr.hitag = adjusthp(85);
			break;
		case 1:
			spr.extra = 20;
			spr.hitag = adjusthp(85);
			break;
		case 2:
			spr.extra = 10;
			spr.hitag = adjusthp(45);
			break;
		case 3:
			spr.extra = 0;
			spr.hitag = adjusthp(15);
			break;
		}
		spr.xrepeat = 35;
		spr.yrepeat = 35;
		spr.picnum = NEWGUY;
	}
}
	
static void skirmishnewguy(PLAYER& plr, DWHActor* actor)
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

	checksector6(actor);
}

static void searchnewguy(PLAYER& plr, DWHActor* actor)
{
	aisearch(plr, actor, false);
	checksector6(actor);
}
	
static void nukednewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	chunksofmeat(plr, actor,spr.x, spr.y, spr.z, spr.sectnum, spr.ang);
	trailingsmoke(actor,false);
	SetNewStatus(actor, DIE);
}
	
static void painnewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum = NEWGUY;
		spr.ang = plr.angle.ang.asbuild();
		SetNewStatus(actor, FLEE);
	}

	aimove(actor);
	processfluid(actor, zr_florHit, false);
	SetActorPos(actor, &spr.pos);
}
	
static void facenewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	boolean cansee = ::cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum);

	if (cansee && plr.invisibletime < 0) {
		spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
		if (plr.shadowtime > 0) {
			spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
		}
		else {
			actor->SetOwner(plr.actor());
			SetNewStatus(actor, CHASE);
		}
	}
	else { // get off the wall
		if (actor->GetOwner() == plr.actor()) {
			spr.ang = (((krand() & 512 - 256) + spr.ang) & 2047);
			SetNewStatus(actor, FINDME);
		}
		else if (cansee) SetNewStatus(actor, FLEE);
	}

	if (checkdist(plr, actor))
		SetNewStatus(actor, ATTACK);
}
	
static void fleenewguy(PLAYER& plr, DWHActor* actor)
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
			if (plr.invisibletime < 0)
				spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
			SetNewStatus(actor, FACE);
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
}
	
static void attacknewguy(PLAYER& plr, DWHActor* actor)
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

	SetActorPos(actor, &spr.pos);

	switch (spr.picnum) {
	case NEWGUYCAST:
	case NEWGUYBOW:
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
	case NEWGUYMACE:
	case NEWGUYPUNCH:
		if (spr.lotag == 31) {
			if (checksight(plr, actor))
				if (checkdist(plr, actor)) {
					spr.ang = checksight_ang;
					attack(plr, actor);
				}
		}
		else if (spr.lotag < 0) {
			if (plr.shadowtime > 0) {
				spr.ang = (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
				SetNewStatus(actor, FLEE);
			}
			else
				SetNewStatus(actor, CHASE);
		}
		spr.lotag -= TICSPERFRAME;

		checksector6(actor);
		break;
	}
}
	
static void dienewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;

	if (spr.lotag <= 0) {
		spr.picnum++;
		spr.lotag = 20;

		if (spr.picnum == NEWGUYDEAD) {
			if (difficulty == 4)
				SetNewStatus(actor, RESURECT);
			else {
				kills++;
				SetNewStatus(actor, DEAD);
			}
		}
	}
}
	
static void castnewguy(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	if (spr.lotag < 0) {
		spr.picnum++;
		spr.lotag = 12;
	}

	if (spr.picnum == NEWGUYCAST + 2) {
		spr.extra--;
		spr.picnum = NEWGUY;
		spritesound(S_WISP, actor);
		skullycastspell(plr, actor);
		SetNewStatus(actor, CHASE);
	}
	if (spr.picnum == NEWGUYBOW + 2) {
		spr.extra--;
		spr.picnum = NEWGUY;
		spritesound(S_PLRWEAPON3, actor);
		newguyarrow(actor, plr);
		SetNewStatus(actor, CHASE);
	}
	checksector6(actor);
}

static void newguyarrow(DWHActor* actor, PLAYER& plr) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, JAVLIN);
	auto& spawned = spawnedactor->s();
	
	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z - (40 << 8);

	spawned.cstat = 21;

	spawned.picnum = WALLARROW;
	spawned.ang = (((spr.ang + 2048 + 96) - 512) & 2047);
	spawned.xrepeat = 24;
	spawned.yrepeat = 24;
	spawned.clipdist = 32;

	spawned.extra = spr.ang;
	spawned.shade = -15;
	spawned.xvel = ((krand() & 256) - 128);
	spawned.yvel = ((krand() & 256) - 128);

	spawned.zvel = (((plr.z + (8 << 8) - spr.z) << 7) / ksqrt((plr.x - spr.x) * (plr.x - spr.x) + (plr.y - spr.y) * (plr.y - spr.y)));

	spawned.zvel += ((krand() % 256) - 128);

	spawnedactor->SetOwner(actor);
	spawned.lotag = 1024;
	spawned.hitag = 0;
	spawned.pal = 0;
	spr.backuploc();
}

void createNewGuyAI() {
	//picanm[NEWGUYDIE] = 0;
	//picanm[NEWGUYDIE + 3] = 0;
	auto& e = enemy[NEWGUYTYPE];
	e.info.Init(35, 35, 1024 + 256, 120, 0, 48, false, 90, 0);
	e.info.getAttackDist = [](EnemyInfo& e, SPRITE& spr)
	{
		int out = e.attackdist;
		switch (spr.picnum) {
		case NEWGUY:
		case NEWGUYMACE:
		case NEWGUYCAST:
		case NEWGUYBOW:
			if (spr.extra > 10)
				out = 2048 << 1;
			else out = 1024 + 256;
			break;
		case NEWGUYPUNCH:
			out = 1024 + 256;
			break;
		default:
			out = 512;
			break;
		}

		return out;
	};

	e.info.getHealth = [](EnemyInfo& e, SPRITE& spr)
	{
		switch (spr.picnum) {
		case NEWGUYSTAND:
		case NEWGUYKNEE:
			return adjusthp(50);
		case NEWGUYCAST:
			return adjusthp(85);
		case NEWGUYBOW:
			return adjusthp(85);
		case NEWGUYMACE:
			return adjusthp(45);
		case NEWGUYPUNCH:
			return adjusthp(15);
		}

		return adjusthp(e.health);
	};
	e.stand = standnewguy;
	e.chase = chasenewguy;
	e.resurect = resurectnewguy;
	e.skirmish = skirmishnewguy;
	e.search = searchnewguy;
	e.nuked = nukednewguy;
	e.pain = painnewguy;
	e.face = facenewguy;
	e.flee = fleenewguy;
	e.attack = attacknewguy;
	e.die = dienewguy;
	e.cast = castnewguy;
}


void premapNewGuy(DWHActor* actor) {
	SPRITE& spr = actor->s();
	spr.detail = NEWGUYTYPE;

	enemy[NEWGUYTYPE].info.set(spr);

	switch (spr.picnum) {
	case NEWGUYSTAND:
	case NEWGUYKNEE:
		ChangeActorStat(actor, STAND);
		if (spr.picnum == NEWGUYSTAND)
			spr.extra = 20;
		else
			spr.extra = 30;
		break;
	case NEWGUYCAST:
	case NEWGUYBOW:
	case NEWGUYMACE:
	case NEWGUYPUNCH:
	case NEWGUY:
		switch (spr.picnum) {
		case NEWGUYCAST:
			spr.extra = 30;
			break;
		case NEWGUYBOW:
			spr.extra = 20;
			break;
		case NEWGUYMACE:
			spr.extra = 10;
			break;
		case NEWGUYPUNCH:
			spr.extra = 0;
			break;
		}
		ChangeActorStat(actor, FACE);
		spr.picnum = NEWGUY;
	}
}

END_WH_NS
