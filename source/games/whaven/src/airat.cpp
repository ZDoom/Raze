#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void chaserat(PLAYER& plr, DWHActor* actor)
{
	SetNewStatus(actor, FLEE);
}
	
static void searchrat(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();
	spr.ang = (short) (((krand() & 512 - 256) + spr.ang + 1024) & 2047);
	SetNewStatus(actor, FLEE);
}
	
static void facerat(PLAYER& plr, DWHActor* actor)
{
	SPRITE& spr = actor->s();

	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	spr.ang = (short) (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
	spr.owner = sprite[plr.spritenum].owner;
	SetNewStatus(actor, FLEE);
}
	
static void dierat(PLAYER& plr, DWHActor* actor)
{
	DeleteActor(actor);
}
	
static void fleerat(PLAYER& plr, DWHActor* actor)
{
	int i = actor->GetSpriteIndex();
	SPRITE& spr = actor->s();

	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	auto moveStat = aimove(actor);
	if (moveStat.type == kHitFloor)
	{
		spr.ang = (short)((spr.ang + 1024) & 2047);
		return;
	}

	if (moveStat.type == kHitWall) {
		WALL& wal = wall[moveStat.index];
		short wallang = (short)((getangle(wall[wal.point2].x - wal.x, wall[wal.point2].y - wal.y) + 512)
			& 2047);
		spr.ang = (short)(krand() & 512 - 256 + wallang);
	}

	if (moveStat.type == kHitSprite) {
		SPRITE& sp = moveStat.actor->s();
		spr.owner = moveStat.actor->GetSpriteIndex();
		spr.ang = getangle(sp.x - spr.x, sp.y - spr.y);
		spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
	}

	if (abs(plr.x - spr.x) <= 1024 && abs(plr.y - spr.y) <= 1024) {
		spr.owner = sprite[plr.spritenum].owner;
		SetNewStatus(actor, FACE);
	}

	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);


	if (checksector6(actor))
		return;

	processfluid(actor, zr_florHit, false);

	//				switch (checkfluid(actor, zr_florHit)) {
	//				case TYPELAVA:
	//				case TYPEWATER:
	//					spr.z += tileHeight(spr.picnum) << 5;
	//					break;
	//				}

	if (zr_florHit.type == kHitSector && (spr.sector()->floorpicnum == LAVA
		|| spr.sector()->floorpicnum == LAVA2
		|| spr.sector()->floorpicnum == LAVA1 || spr.sector()->floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			SetNewStatus(actor, DIE);
	}

	SetActorPos(actor, &spr.pos);
}


void createRatAI() {
	auto& e = enemy[RATTYPE];
	e.info.Init(32, 32, 512, 120, 0, 32, false, 0, 0);
	e.info.getHealth = [](EnemyInfo&, SPRITE& spr) -> short
	{
		return 10;
	};
	e.chase = chaserat;
	e.search = searchrat;
	e.face = facerat;
	e.die = dierat;
	e.flee = fleerat;
}
	
void premapRat(short i) {
	SPRITE& spr = sprite[i];

	spr.detail = RATTYPE;
	enemy[RATTYPE].info.set(spr);
	changespritestat(i, FACE);

	spr.shade = 12;
	spr.pal = 5;
}

END_WH_NS
