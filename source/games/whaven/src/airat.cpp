#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static void chaserat(PLAYER& plr, short i) {
	newstatus(i, FLEE);
}
	
static void searchrat(PLAYER& plr, short i) {
	sprite[i].ang = (short) (((krand() & 512 - 256) + sprite[i].ang + 1024) & 2047);
	newstatus(i, FLEE);
}
	
static void facerat(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.ang = getangle(plr.x - spr.x, plr.y - spr.y);
	spr.ang = (short) (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
	spr.owner = sprite[plr.spritenum].owner;
	newstatus(i, FLEE);
}
	
static void dierat(PLAYER& plr, short i) {
	deletesprite(i);
}
	
static void fleerat(PLAYER& plr, short i) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;
	short osectnum = spr.sectnum;

	int movestat = aimove(i);
	if ((movestat & kHitTypeMask) == kHitFloor)
	{
		spr.ang = (short)((spr.ang + 1024) & 2047);
		return;
	}

	if ((movestat & kHitTypeMask) == kHitWall) {
		WALL& wal = wall[movestat & kHitIndexMask];
		short wallang = (short)((getangle(wall[wal.point2].x - wal.x, wall[wal.point2].y - wal.y) + 512)
			& 2047);
		spr.ang = (short)(krand() & 512 - 256 + wallang);
	}

	if ((movestat & kHitTypeMask) == kHitSprite) {
		SPRITE& sp = sprite[movestat & kHitIndexMask];
		spr.owner = (short)(movestat & kHitIndexMask);
		spr.ang = getangle(sp.x - spr.x, sp.y - spr.y);
		spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047);
	}

	if (abs(plr.x - spr.x) <= 1024 && abs(plr.y - spr.y) <= 1024) {
		spr.owner = sprite[plr.spritenum].owner;
		newstatus(i, FACE);
	}

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);


	if (checksector6(i))
		return;

	processfluid(i, zr_florhit, false);

	//				switch (checkfluid(i, zr_florhit)) {
	//				case TYPELAVA:
	//				case TYPEWATER:
	//					spr.z += tileHeight(spr.picnum) << 5;
	//					break;
	//				}

	if ((zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == LAVA2
		|| sector[spr.sectnum].floorpicnum == LAVA1 || sector[spr.sectnum].floorpicnum == ANILAVA)) {
		spr.hitag--;
		if (spr.hitag < 0)
			newstatus(i, DIE);
	}

	setsprite(i, spr.x, spr.y, spr.z);
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
