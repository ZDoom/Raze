#include "ns.h"
#include "wh.h"
#include "i_net.h"

BEGIN_WH_NS

Enemy enemy[MAXTYPES];


int checksight_ang = 0;

void skeletonChill(PLAYER& plr, DWHActor* i);
void goblinChill(PLAYER& plr, DWHActor* i);


void createDemonAI();
void createDevilAI();
void createDragonAI();
void createFishAI();
void createFatwitchAI();
void createFredAI();
void createGoblinAI();
void createGonzoAI();
void createGronAI();
void createGuardianAI();
void createImpAI();
void createJudyAI();
void createKatieAI();
void createKoboldAI();
void createKurtAI();
void createMinotaurAI();
void createNewGuyAI();
void createRatAI();
void createSkeletonAI();
void createSkullyAI();
void createSpiderAI();
void createWillowAI();

void judyOperate(PLAYER& plr);
void gonzoProcess(PLAYER& plr);
void goblinWarProcess(PLAYER& plr);
void dragonProcess(PLAYER& plr);
void willowProcess(PLAYER& plr);


void initAI()
{


	/*
	 * ai attack ai resurect from enemyInfo goblin imp etc patrol point search
	 *
	 *
	 * AMBUSH -> LAND
	 *
	 * case KOBOLD: case IMP: case MINOTAUR: case SKELETON: case GRONSW: case
	 * NEWGUY:
	 *
	 * PATROL
	 */

	if (isWh2())
		createImpAI();
	else
		createGoblinAI();
	createDevilAI();
	createSkeletonAI();
	createDragonAI();
	createKoboldAI();
	createGuardianAI();
	createWillowAI();
	createRatAI();
	createFredAI();
	createFishAI();
	createSpiderAI();
	createMinotaurAI();
	createGronAI();
	createFatwitchAI();
	createSkullyAI();
	createJudyAI();
	createDemonAI();
	createKatieAI();
	createNewGuyAI();
	createGonzoAI();
	createKurtAI(); // kurt must be initialized after gonzo
}

void aiInit() {
	WHLinearSpriteIterator it;
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();
		int pic = spr.picnum;
		switch (spr.picnum) {
		default:
			if (pic == SKELETON || pic == HANGMAN) {
				premapSkeleton(i);
				killcnt++;
			}
			else if (pic == GUARDIAN) {
				premapGuardian(i);
				killcnt++;
			}
			else if (pic == WILLOW) {
				premapWillow(i);
				killcnt++;
			}
			else if (pic == RAT) {
				premapRat(i);
			}
			else if (pic == FISH) {
				premapFish(i);
			}
			else if (pic == GRONHAL || pic == GRONMU || pic == GRONSW) {
				premapGron(i);
				killcnt++;
			}
			break;
		case GOBLIN: // IMP
		case GOBLINSTAND:
		case GOBLINCHILL:
			killcnt++;
			if (isWh2() && spr.picnum == IMP) {
				premapImp(i);
				break;
			}

			if (!isWh2())
				premapGoblin(i);
			break;
		case DEVIL:
		case DEVILSTAND:
			if (sprite[i].pal != 8) {
				premapDevil(i);
				killcnt++;
			}
			break;
		case DRAGON:
			premapDragon(i);
			killcnt++;
			break;
		case KOBOLD:
			premapKobold(i);
			killcnt++;
			break;
		case FRED:
		case FREDSTAND:
			premapFred(i);
			killcnt++;
			break;
		case SPIDER:
			premapSpider(i);
			killcnt++;
			break;
		case MINOTAUR:
			premapMinotaur(i);
			killcnt++;
			break;
		case FATWITCH:
			premapFatwitch(i);
			killcnt++;
			break;
		case SKULLY:
			premapSkully(i);
			killcnt++;
			break;
		case JUDY:
		case JUDYSIT:
			premapJudy(i);
			killcnt++;
			break;
		case DEMON:
			premapDemon(i);
			killcnt++;
			break;
		case KATIE:
			premapKatie(i);
			killcnt++;
			break;
		case KURTSTAND:
		case KURTKNEE:
			premapKurt(i);
			killcnt++;
			break;
		case NEWGUYSTAND:
		case NEWGUYKNEE:
		case NEWGUYCAST:
		case NEWGUYBOW:
		case NEWGUYMACE:
		case NEWGUYPUNCH:
		case NEWGUY:
			premapNewGuy(i);
			killcnt++;
			break;
		case KURTAT:
		case KURTPUNCH:
		case GONZOCSW:
		case GONZOGSW:
		case GONZOGHM:
		case GONZOGSH:
			premapGonzo(i);
			killcnt++;
			break;
		}
	}
}

void aiProcess() {

	PLAYER& plr = player[0];

	//		short daang = plr.angle.ang.asbuild();
	//		int daz2 = -MulScale(plr.horizon.horiz.asq16(), 2000, 16);
	//		hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
	//				bcos(daang), // X vector of 3D ang
	//				bsin(daang), // Y vector of 3D ang
	//				daz2, // Z vector of 3D ang
	//				pHitInfo, CLIPMASK0);
	//		
	//		if(pHitInfo.hitsprite != -1)
	//		{
	//			int sprid = pHitInfo.hitsprite;
	//		}

	judyOperate(plr);
	gonzoProcess(plr);
	goblinWarProcess(plr);
	dragonProcess(plr);
	willowProcess(plr);

	short i;

	WHStatIterator it(PATROL);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		i = actor->GetSpriteIndex();
		Collision moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
			(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
		if (zr_florz > spr.z + (48 << 8)) {
			SetActorPos(actor, &spr.pos);
			moveStat.type = -1;
		}
		else {
			spr.z = zr_florz;
		}
		WHSectIterator it(spr.sectnum);
		while (auto sectactor = it.Next())
		{
			SPRITE& tspr = sectactor->s();
			int j = sectactor->GetSpriteIndex();

			if (tspr.picnum == PATROLPOINT) {
				int dx = abs(spr.x - tspr.x); // x distance to sprite
				int dy = abs(spr.y - tspr.y); // y distance to sprite
				int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
				int dh = tileHeight(tspr.picnum) >> 4; // height of sprite
				if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
					spr.ang = tspr.ang;
				}
			}
		}
		if (bcos(spr.ang) * (plr.x - spr.x)	+ bsin(spr.ang) * (plr.y - spr.y) >= 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
				spr.sectnum)) {
				SetNewStatus(actor, CHASE);
			}
		}
		else if (moveStat.type != kHitNone) {
			if (moveStat.type == kHitWall) { // hit a wall
				actoruse(i);
			}
			SetNewStatus(actor, FINDME);
		}
	}

	it.Reset(CHASE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].chase != nullptr)
			enemy[spr.detail].chase(plr, actor);
	}

	it.Reset(RESURECT);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].resurect != nullptr) {
			enemy[spr.detail].resurect(plr, actor);
		}
	}

	it.Reset(FINDME);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].search != nullptr)
			enemy[spr.detail].search(plr, actor);
	}

	it.Reset(NUKED);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (spr.picnum == ZFIRE) {
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag <= 0)
				DeleteActor(actor);
		}
		else {
			if (enemy[spr.detail].nuked != nullptr)
				enemy[spr.detail].nuked(plr, actor);
		}
	}

	it.Reset(FROZEN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].frozen != nullptr)
			enemy[spr.detail].frozen(plr, actor);
	}

	it.Reset(PAIN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].pain != nullptr)
			enemy[spr.detail].pain(plr, actor);
	}

	it.Reset(FACE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].face != nullptr)
			enemy[spr.detail].face(plr, actor);
	}

	it.Reset(ATTACK);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (isWh2() && attacktheme == 0) {
			attacktheme = 1;
			startsong((rand() % 2) + 2);
		}

		if (enemy[spr.detail].attack != nullptr)
			enemy[spr.detail].attack(plr, actor);
	}

	it.Reset(FLEE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].flee != nullptr)
			enemy[spr.detail].flee(plr, actor);
	}

	it.Reset(CAST);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].cast != nullptr)
			enemy[spr.detail].cast(plr, actor);
	}

	it.Reset(DIE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].die != nullptr)
			enemy[spr.detail].die(plr, actor);
	}

	it.Reset(SKIRMISH);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].skirmish != nullptr)
			enemy[spr.detail].skirmish(plr, actor);
	}

	it.Reset(STAND);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		if (enemy[spr.detail].stand != nullptr)
			enemy[spr.detail].stand(plr, actor);
	}

	it.Reset(CHILL);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();

		switch (spr.detail) {
		case GOBLINTYPE:
			goblinChill(plr, actor);
			break;
		case SKELETONTYPE:
			skeletonChill(plr, actor);
			break;
		}
	}

	it.Reset(DEAD);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
		switch (checkfluid(actor, zr_florHit)) {
		case TYPELAVA:
		case TYPEWATER:
			spr.z = zr_florz + (tileHeight(spr.picnum) << 5);
			break;
		}
	}
}

Collision aimove(DWHActor* actor)
{
	auto& spr = actor->s();
	int ox = spr.x;
	int oy = spr.y;
	int oz = spr.z;
	//		short osect = spr.sectnum;

	Collision moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
		(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	if (((zr_florz - oz) >> 4) > tileHeight(spr.picnum) + (spr.yrepeat << 2)
		|| moveStat.type == kHitWall) {

		SetActorPos(actor, ox, oy, oz);

		if (moveStat.type != kHitWall) {
			if (isWh2())
				spr.z += WH2GRAVITYCONSTANT;
			else
				spr.z += GRAVITYCONSTANT;
			return zr_florHit;
		}
	}

	spr.z = zr_florz;

	return moveStat;
}

Collision aifly(DWHActor* actor) {
	SPRITE& spr = actor->s();
	Collision moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
		(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	spr.z -= TICSPERFRAME << 8;
	short ocs = spr.cstat;
	spr.cstat = 0;
	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.cstat = ocs;
	if (spr.z > zr_florz)
		spr.z = zr_florz;
	if (spr.z - (tileHeight(spr.picnum) << 7) < zr_ceilz)
		spr.z = zr_ceilz + (tileHeight(spr.picnum) << 7);

	return moveStat;
}


void aisearch(PLAYER& plr, DWHActor* actor, boolean fly) {
	SPRITE& spr = actor->s();
	spr.lotag -= TICSPERFRAME;

	//		if (plr.invisibletime > 0) {
	//			SetNewStatus(actor, FACE);
	//			return;
	//		}

	short osectnum = spr.sectnum;

	Collision moveStat;
	if (fly)
		moveStat = aifly(actor);
	else
		moveStat = aimove(actor);

	if (checkdist(plr, actor)) {
		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			SetNewStatus(actor, FLEE);
		}
		else
			SetNewStatus(actor, ATTACK);
		return;
	}

	if (moveStat.type != kHitNone) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && spr.lotag < 0) {
			spr.ang = (short)((spr.ang + 1024) & 2047);
			SetNewStatus(actor, FLEE);
			return;
		}
		if (spr.lotag < 0) {
			if (krand() % 100 > 50)
				spr.ang = (short)((spr.ang + 512) & 2047);
			else
				spr.ang = (short)((spr.ang + 1024) & 2047);

			spr.lotag = 30;
		}
		else {
			spr.ang += (TICSPERFRAME << 4) & 2047;
		}
	}

	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
		spr.sectnum) && moveStat.type == kHitNone && spr.lotag < 0) {
		SetNewStatus(actor, FACE);
		return;
	}

	if ((spr.sectnum != osectnum) && (spr.sector()->lotag == 10))
		warpsprite(actor);

	processfluid(actor, zr_florHit, fly);

	SetActorPos(actor, &spr.pos);
}

boolean checksector6(DWHActor* actor) {
	SPRITE& spr = actor->s();
	if (spr.sector()->floorz - (32 << 8) < spr.sector()->ceilingz) {
		if (spr.sector()->lotag == 6)
			SetNewStatus(actor, DIE);
		else {
			DeleteActor(actor);
			return true;
		}
	}

	return false;
}

int checkfluid(DWHActor* actor, Collision& florHit) 
{
	SPRITE& spr = actor->s();
	if (isValidSector(spr.sectnum) && florHit.type == kHitSector && (spr.sector()->floorpicnum == WATER
		/* || spr.sector()->floorpicnum == LAVA2 */ || spr.sector()->floorpicnum == LAVA
		|| spr.sector()->floorpicnum == SLIME || spr.sector()->floorpicnum == FLOORMIRROR
		/*
		 * || spr.sector()->floorpicnum == LAVA1 ||
		 * spr.sector()->floorpicnum == ANILAVA
		 */)) {
		if (spr.sector()->floorpicnum == WATER || spr.sector()->floorpicnum == SLIME
			|| spr.sector()->floorpicnum == FLOORMIRROR) {
			return TYPEWATER;
		}
		else {
			return TYPELAVA;
		}
	}

	return TYPENONE;
}

void processfluid(DWHActor* actor, Collision& florHit, boolean fly) {
	SPRITE& spr = actor->s();
	switch (checkfluid(actor, florHit)) {
	case TYPELAVA:
		if (!fly) {
			spr.z += tileHeight(spr.picnum) << 5;
			trailingsmoke(actor,true);
			makemonstersplash(LAVASPLASH, actor->GetSpriteIndex());
		}
		break;
	case TYPEWATER:
		if (!fly) {
			spr.z += tileHeight(spr.picnum) << 5;
			if (krand() % 100 > 60)
				makemonstersplash(SPLASHAROO, actor->GetSpriteIndex());
		}
		break;
	}
}

void castspell(PLAYER& plr, DWHActor* actor) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, MISSILE);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	if (isWh2() || spr.picnum == SPAWNFIREBALL)
		spawned.z = spr.z - ((tileHeight(spr.picnum) >> 1) << 8);
	else
		spawned.z = getflorzofslope(spr.sectnum, spr.x, spr.y) - ((tileHeight(spr.picnum) >> 1) << 8);
	spawned.cstat = 0; // Hitscan does not hit other bullets
	spawned.picnum = MONSTERBALL;
	spawned.shade = -15;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	if (spr.picnum == SPAWNFIREBALL)
		spawned.ang = (short)((getangle(plr.x - spawned.x, plr.y - spawned.y) + 2048) & 2047);
	else
		spawned.ang = (short)(((getangle(plr.x - spawned.x, plr.y - spawned.y) + (krand() & 15)
			- 8) + 2048) & 2047);
	spawned.xvel = bcos(spawned.ang, -6);
	spawned.yvel = bsin(spawned.ang, -6);

	int discrim = ksqrt((plr.x - spawned.x) * (plr.x - spawned.x) + (plr.y - spawned.y) * (plr.y - spawned.y));
	if (discrim == 0)
		discrim = 1;
	if (isWh2())
		spawned.zvel = (short)(((plr.z + (8 << 8) - spawned.z) << 7) / discrim);
	else
		spawned.zvel = (short)(((plr.z + (48 << 8) - spawned.z) << 7) / discrim);

	spawned.owner = (short)actor->GetSpriteIndex();
	spawned.clipdist = 16;
	spawned.lotag = 512;
	spawned.hitag = 0;
	spawned.backuploc();
}

void skullycastspell(PLAYER& plr, int i) {
	auto& spr = sprite[i];
	auto spawnedactor = InsertActor(spr.sectnum, MISSILE);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	if (spr.picnum == SPAWNFIREBALL)
		spawned.z = spr.z - ((tileHeight(spr.picnum) >> 1) << 8);
	else
		spawned.z = getflorzofslope(spr.sectnum, spr.x, spr.y) - ((tileHeight(spr.picnum) >> 1) << 8);
	spawned.cstat = 0; // Hitscan does not hit other bullets
	spawned.picnum = PLASMA;
	spawned.shade = -15;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	if (spr.picnum == SPAWNFIREBALL)
		spawned.ang = (short)((getangle(plr.x - spawned.x, plr.y - spawned.y) + 2048) & 2047);
	else
		spawned.ang = (short)(((getangle(plr.x - spawned.x, plr.y - spawned.y) + (krand() & 15)
			- 8) + 2048) & 2047);
	spawned.xvel = bcos(spawned.ang, -6);
	spawned.yvel = bsin(spawned.ang, -6);

	int discrim = ksqrt((plr.x - spawned.x) * (plr.x - spawned.x) + (plr.y - spawned.y) * (plr.y - spawned.y));
	if (discrim == 0)
		discrim = 1;
	spawned.zvel = (short)(((plr.z + (48 << 8) - spawned.z) << 7) / discrim);

	spawned.owner = (short)i;
	spawned.clipdist = 16;
	spawned.lotag = 512;
	spawned.hitag = 0;
	spawned.pal = 7;
	spawned.backuploc();
}

void attack(PLAYER& plr, DWHActor* actor) {
	int s = 0;
	if (plr.invincibletime > 0 || plr.godMode)
		return;

	if (plr.treasure[TADAMANTINERING] == 1 && (krand() & 1) != 0)
		return;

	//		if ((krand() & (15 < plr.armortype ? 11 : 10)) != 0)
	//			return;
	auto& spr = actor->s();
	auto& pspr = sprite[plr.spritenum];

	if (!droptheshield && plr.shieldpoints > 0 && plr.selectedgun > 0 && plr.selectedgun < 5) {
		short a = getangle(spr.x - plr.x, spr.y - plr.y);
		auto ang = plr.angle.ang.asbuild();
		if ((a < ang && ang - a < 128) || (a > ang && ((ang + a) & 2047) < 128)) {
			if (krand() % 100 > 80) {
				spritesound(S_SWORD1 + krand() % 3, plr.actor());
				return;
			}
			else {
				s = krand() % 50;
				plr.shieldpoints -= s;
				if (krand() % 100 > 50) {
					spritesound(S_SWORD1 + krand() % 3, plr.actor());
					return;
				}
			}
		}
		if (plr.shieldpoints <= 0) {
			showmessage("Shield useless", 360);
		}
	}

	int k = 5;
	if (!isWh2()) {
		k = krand() % 100;
		if (k > (plr.armortype << 3))
			k = 15;
		else
			k = 5;
	}

	switch (spr.detail) {
	case SPIDER:
		k = 5;
		break;
	case FISHTYPE:
	case RATTYPE:
		k = 3;
		break;
	case SKELETONTYPE:
		spritesound(S_RIP1 + (krand() % 3), actor);
		if ((krand() % 2) != 0)
			spritesound(S_GORE1 + (krand() % 4), actor);
		if ((krand() % 2) != 0)
			spritesound(S_BREATH1 + (krand() % 6), actor);

		if (isWh2())
			k = (krand() % 5) + 5;
		else
			k >>= 2;
		break;
	case KATIETYPE: // damage 5 - 50
		spritesound(S_DEMONTHROW, actor);
		k = (krand() % 45) + 5;
		break;

	case DEVILTYPE:
		spritesound(S_DEMONTHROW, actor);
		if (!isWh2())
			k >>= 2;
		break;

	case KOBOLDTYPE:
		spritesound(S_GENSWING, actor);
		if ((krand() % 10) > 4) {
			spritesound(S_KOBOLDHIT, plr.actor());
			spritesound(S_BREATH1 + (krand() % 6), plr.actor());
		}
		if (isWh2())
			k = (krand() % 5) + 5;
		else
			k >>= 2;
		break;
	case FREDTYPE:

		/* Sounds for Fred (currently copied from Goblin) */
		spritesound(S_GENSWING, actor);
		if (rand() % 10 > 4)
			spritesound(S_SWORD1 + (rand() % 6), actor);

		k >>= 3;
		break;
	case IMPTYPE:
		if (!isWh2())
			break;
		spritesound(S_RIP1 + (krand() % 3), actor);
		if ((krand() % 2) != 0) {
			spritesound(S_GORE1 + (krand() % 4), actor);
		}
		if ((krand() % 2) != 0) {
			spritesound(S_BREATH1 + (krand() % 6), actor);
		}

		k = (krand() % 5) + 5;
		if (k > 8) {
			plr.poisoned = 1;
		}
		break;
	case GOBLINTYPE:
		if (isWh2())
			break;

		spritesound(S_GENSWING, actor);
		if ((krand() % 10) > 4)
			spritesound(S_SWORD1 + (krand() % 6), actor);
		k >>= 2;
		break;
	case NEWGUYTYPE:
		if (spr.picnum == NEWGUYMACE) { // damage 5 - 20
			spritesound(S_PLRWEAPON2, actor);
			if (krand() % 10 > 4) {
				spritesound(S_KOBOLDHIT, plr.actor());
				spritesound(S_BREATH1 + (krand() % 6), plr.actor());
			}
			k = (krand() % 15) + 5;
			break;
		}
	case KURTTYPE:
	case GONZOTYPE:
		spritesound(S_GENSWING, actor);
		if (spr.picnum == GONZOCSWAT || spr.picnum == GONZOGSWAT) { // damage 5 - 15
			if (krand() % 10 > 6)
				spritesound(S_SWORD1 + (krand() % 6), actor);
			k = (krand() % 15) + 5;
		}
		else if (spr.picnum == GONZOGHMAT) { // damage 5 - 15
			if (krand() % 10 > 6)
				spritesound(S_SWORD1 + (krand() % 6), actor);
			k = (krand() % 10) + 5;
		}
		else if (spr.picnum == GONZOGSHAT) { // damage 5 - 20
			if (krand() % 10 > 3)
				spritesound(S_SWORD1 + (krand() % 6), actor);
			k = (krand() % 15) + 5;
		}
		else if (spr.picnum == KURTAT) { // damage 5 - 15
			spritesound(S_GENSWING, actor);
			if (krand() % 10 > 3) {
				spritesound(S_SWORD1 + (krand() % 6), actor);
			}
			k = (krand() % 10) + 5;
		}
		else {
			spritesound(S_GENSWING, actor);
			if (krand() % 10 > 4) {
				spritesound(S_SOCK1 + (krand() % 4), plr.actor());
				spritesound(S_BREATH1 + (krand() % 6), plr.actor());
			}
			k = (krand() % 4) + 1;
		}
		break;

	case GRONTYPE:
		if (spr.picnum != GRONSWATTACK)
			break;

		if (isWh2()) {
			k = (krand() % 20) + 5;
			if (spr.shade > 30) {
				k += krand() % 10;
			}
		}
		else {
			if (spr.shade > 30)
				k >>= 1;
		}
		spritesound(S_GENSWING, actor);
		if ((krand() % 10) > 3)
			spritesound(S_SWORD1 + (krand() % 6), actor);

		break;
	case MINOTAURTYPE:
		spritesound(S_GENSWING, actor);
		if (krand() % 10 > 4)
			spritesound(S_SWORD1 + (krand() % 6), actor);
		if (isWh2())
			k = (krand() % 25) + 5;
		break;
	}

	if (plr.shieldpoints > 0) {
		if (s > k)
			k = 0;
		else
			k -= s;
	}

	int a;
	switch (plr.armortype) {
	case 0: // none
		addhealth(plr, -k);
		break;
	case 1: // leather
		a = krand() % 5;
		if (a > k) {
			k = 0;
		}
		else {
			k -= a;
		}
		addarmor(plr, -a);
		addhealth(plr, -k);
		break;
	case 2: // chain
		a = krand() % 10;
		if (a > k) {
			k = 0;
		}
		else {
			k -= a;
		}
		addarmor(plr, -a);
		addhealth(plr, -k);
		break;
	case 3: // plate
		a = krand() % 20;
		if (a > k) {
			k = 0;
		}
		else {
			k -= a;
		}
		addarmor(plr, -a);
		addhealth(plr, -k);
		break;
	}

	startredflash(3 * k);

	if (k == 0)
		k = 1;

	k = krand() % k;

	damage_angvel += k << 3;
	damage_svel += k << 3;
	damage_vel -= k << 3;

	plr.hvel += k << 2;
}

int checkmove(DWHActor* actor, int dax, int day) {
	auto& spr = actor->s();
	Collision moveStat = movesprite(actor, dax, day, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	if (moveStat.type != kHitNone)
		spr.ang = (short)((spr.ang + TICSPERFRAME) & 2047);

	return moveStat.legacyVal;
}

boolean checkdist(PLAYER& plr, DWHActor* actor) {
	if (plr.invisibletime > 0 || plr.health <= 0)
		return false;

	return checkdist(actor, plr.x, plr.y, plr.z);
}
	
boolean checkdist(DWHActor* actor, int x, int y, int z) {
	SPRITE& spr = actor->s();

	int attackdist = 512;
	int attackheight = 120;
	if (spr.detail > 0) {
		attackdist = enemy[spr.detail].info.getAttackDist(enemy[spr.detail].info, spr);
		attackheight = enemy[spr.detail].info.attackheight;
	}

	switch (spr.picnum) {
	case LFIRE:
	case SFIRE:
		attackdist = 1024;
		break;
	}

	if ((abs(x - spr.x) + abs(y - spr.y) < attackdist)
		&& (abs((z >> 8) - ((spr.z >> 8) - (tileHeight(spr.picnum) >> 1))) <= attackheight))
		return true;

	return false;
}

boolean checksight(PLAYER& plr, DWHActor* actor) {
	auto& spr = actor->s();
	if (plr.invisibletime > 0) {
		checksight_ang = ((krand() & 512) - 256) & 2047;
		return false;
	}

	if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
		spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum) && plr.invisibletime < 0) {
		checksight_ang = (getangle(plr.x - spr.x, plr.y - spr.y) & 2047);
		if (((spr.ang + 2048 - checksight_ang) & 2047) < 1024)
			spr.ang = (short)((spr.ang + 2048 - (TICSPERFRAME << 1)) & 2047);
		else
			spr.ang = (short)((spr.ang + (TICSPERFRAME << 1)) & 2047);

		return true;
	}
	else
		checksight_ang = 0;

	return false;
}

void monsterweapon(int i) {

	if (sprite[i].shade > 20)
		return;

	if (sprite[i].picnum == SKELETONDEAD || sprite[i].picnum == KOBOLDDEAD)
		return;

	if ((krand() % 100) < 75)
		return;

	auto spawnedactor = InsertActor(sprite[i].sectnum, (short)0);
	auto& weap = spawnedactor->s();

	weap.x = sprite[i].x;
	weap.y = sprite[i].y;
	weap.z = sprite[i].z - (24 << 8);
	weap.shade = -15;
	weap.cstat = 0;
	weap.cstat &= ~3;
	weap.pal = 0;
	weap.backuploc();

	int type = (krand() % 4);
	weap.picnum = (short)(FLASKBLUE + type);
	weap.detail = (short)(FLASKBLUETYPE + type);
	weap.xrepeat = 25;
	weap.yrepeat = 20;

	switch (sprite[i].picnum) {
	case NEWGUYDEAD:
		weap.xrepeat = 25;
		weap.yrepeat = 20;
		if (weap.extra < 20) {
			weap.picnum = WEAPON2;
			weap.detail = WEAPON2TYPE;
		}
		else {
			weap.picnum = QUIVER;
			weap.detail = QUIVERTYPE;
		}

		weap.pal = 0;
		break;

	case MINOTAURDEAD:
		weap.xrepeat = 25;
		weap.yrepeat = 20;
		if (!isWh2()) {
			if (krand() % 100 > 50) {
				weap.picnum = WEAPON4;
			}
			else {
				weap.xrepeat = 20;
				weap.yrepeat = 15;
				weap.picnum = WEAPON6;
				weap.detail = WEAPON6TYPE;
			}
		}
		else {
			weap.picnum = WEAPON4;
			weap.detail = WEAPON4TYPE;
		}
		break;

	case GONZOBSHDEAD:
		weap.picnum = GONZOBSHIELD;
		weap.detail = GONZOSHIELDTYPE;
		weap.xrepeat = 12;
		weap.yrepeat = 12;
		break;

	case GONZOCSWDEAD:
		if (weap.extra > 10) {
			weap.picnum = WEAPON6;
			weap.detail = WEAPON6TYPE;
			weap.xrepeat = 25;
			weap.yrepeat = 20;
		}
		else if (weap.extra > 0) {
			weap.picnum = GOBWEAPON;
			weap.detail = GOBWEAPONTYPE;
			weap.xrepeat = 25;
			weap.yrepeat = 20;
		}
		else {
			weap.picnum = WEAPON1;
			weap.detail = WEAPON1TYPE;
			weap.xrepeat = 25;
			weap.yrepeat = 20;
		}
		break;
	case GONZOCSHDEAD:
		weap.picnum = GONZOCSHIELD;
		weap.detail = GONZOSHIELDTYPE;
		weap.xrepeat = 12;
		weap.yrepeat = 12;
		break;

	case GONZOGSWDEAD:
		weap.picnum = WEAPON8;
		weap.detail = WEAPON8TYPE;
		weap.xrepeat = 25;
		weap.yrepeat = 20;
		break;
	case GONZOGHMDEAD:
		weap.picnum = PLATEARMOR;
		weap.detail = PLATEARMORTYPE;
		weap.xrepeat = 26;
		weap.yrepeat = 26;
		break;
	case GONZOGSHDEAD:
		weap.picnum = GONZOGSHIELD;
		weap.detail = GONZOSHIELDTYPE;
		weap.xrepeat = 12;
		weap.yrepeat = 12;
		break;
	case GOBLINDEAD:
		weap.xrepeat = 16;
		weap.yrepeat = 16;
		weap.picnum = GOBWEAPON;
		weap.detail = GOBWEAPONTYPE;
		break;
	default:
		if (sprite[i].picnum == GRONDEAD) {
			if (netgame) {
				weap.x = sprite[i].x;
				weap.y = sprite[i].y;
				weap.z = sprite[i].z - (24 << 8);
				weap.shade = -15;
				weap.cstat = 0;
				weap.cstat &= ~3;
				weap.xrepeat = 25;
				weap.yrepeat = 20;
				int k = krand() % 4;
				switch (k) {
				case 0:
					weap.picnum = WEAPON3;
					weap.detail = WEAPON3TYPE;
					weap.xrepeat = 25;
					weap.yrepeat = 20;
					break;
				case 1:
					weap.picnum = WEAPON5;
					weap.detail = WEAPON5TYPE;
					weap.xrepeat = 25;
					weap.yrepeat = 20;
					break;
				case 2:
					weap.picnum = WEAPON6;
					weap.detail = WEAPON6TYPE;
					weap.xrepeat = 20;
					weap.yrepeat = 15;
					break;
				case 3:
					weap.picnum = SHIELD;
					weap.detail = SHIELDTYPE;
					weap.xrepeat = 32;
					weap.yrepeat = 32;
					break;
				}
			}
			else {
				switch (weap.pal) {
				case 0:
					weap.picnum = WEAPON3;
					weap.detail = WEAPON3TYPE;
					weap.xrepeat = 25;
					weap.yrepeat = 20;
					break;
				case 10:
					weap.picnum = WEAPON5;
					weap.detail = WEAPON5TYPE;
					weap.xrepeat = 25;
					weap.yrepeat = 20;
					break;
				case 11:
					weap.picnum = WEAPON6;
					weap.detail = WEAPON6TYPE;
					weap.xrepeat = 20;
					weap.yrepeat = 15;
					break;
				case 12:
					weap.picnum = SHIELD;
					weap.detail = SHIELDTYPE;
					weap.xrepeat = 32;
					weap.yrepeat = 32;
					break;
				}
			}
			weap.pal = 0;
			break;
		}
		treasurescnt++;
		break;
	}
}

PLAYER* aiGetPlayerTarget(short i) {
	if (sprite[i].owner >= 0 && sprite[i].owner < MAXSPRITES) {
		int playernum = sprite[sprite[i].owner].owner;
		if (playernum >= 4096)
			return &player[playernum - 4096];
	}

	return nullptr;
}

boolean actoruse(short i) {
	SPRITE& spr = sprite[i];
	Neartag nearTag;

	neartag(spr.x, spr.y, spr.z, spr.sectnum, spr.ang, nearTag, 1024, 3);

	if (nearTag.tagsector >= 0) {
		if (sector[nearTag.tagsector].hitag == 0) {
			if (sector[nearTag.tagsector].floorz != sector[nearTag.tagsector].ceilingz) {
				operatesector(player[pyrn], nearTag.tagsector);
				return true;
			}
		}
	}

	return false;
}


END_WH_NS