#include "ns.h"
#include "wh.h"
#include "i_net.h"

BEGIN_WH_NS

Enemy enemy[MAXTYPES];


int checksight_ang = 0;

void skeletonChill(PLAYER& plr, short i);
void goblinChill(PLAYER& plr, short i);


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
	for (short i = 0; i < MAXSPRITES; i++) {
		if (sprite[i].statnum >= MAXSTATUS)
			continue;

		SPRITE& spr = sprite[i];
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

	//		short daang = (short) plr.ang;
	//		int daz2 = (int) (100 - plr.horiz) * 2000;
	//		hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
	//				sintable[(daang + 2560) & 2047], // X vector of 3D ang
	//				sintable[(daang + 2048) & 2047], // Y vector of 3D ang
	//				daz2, // Z vector of 3D ang
	//				pHitInfo, CLIPMASK0);
	//		
	//		if(pHitInfo.hitsprite != -1)
	//		{
	//			int sprid = pHitInfo.hitsprite;
	//			System.err.println(sprite[sprid].statnum);
	//		}

	judyOperate(plr);
	gonzoProcess(plr);
	goblinWarProcess(plr);
	dragonProcess(plr);
	willowProcess(plr);

	short i, nextsprite;

	for (i = headspritestat[PATROL]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		SPRITE& spr = sprite[i];
		short movestat = (short)movesprite((short)i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 3,
			((sintable[spr.ang]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
		if (zr_florz > spr.z + (48 << 8)) {
			setsprite(i, spr.x, spr.y, spr.z);
			movestat = 1;
		}
		else {
			spr.z = zr_florz;
		}
		short j = headspritesect[spr.sectnum];
		while (j != -1) {
			short nextj = nextspritesect[j];
			SPRITE& tspr = sprite[j];
			if (tspr.picnum == PATROLPOINT) {
				int dx = abs(spr.x - tspr.x); // x distance to sprite
				int dy = abs(spr.y - tspr.y); // y distance to sprite
				int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
				int dh = tileHeight(tspr.picnum) >> 4; // height of sprite
				if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
					spr.ang = tspr.ang;
				}
			}
			j = nextj;
		}
		if (sintable[(spr.ang + 2560) & 2047] * (plr.x - spr.x)
			+ sintable[(spr.ang + 2048) & 2047] * (plr.y - spr.y) >= 0) {
			if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
				spr.sectnum)) {
				newstatus(i, CHASE);
			}
		}
		else if (movestat != 0) {
			if ((movestat & 0xc000) == 32768) { // hit a wall
				actoruse(i);
			}
			newstatus(i, FINDME);
		}
	}

	for (i = headspritestat[CHASE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		if (enemy[spr.detail].chase != nullptr)
			enemy[spr.detail].chase(plr, i);
	}

	for (i = headspritestat[RESURECT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		if (enemy[spr.detail].resurect != nullptr) {
			enemy[spr.detail].resurect(plr, i);
		}
	}

	for (i = headspritestat[FINDME]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].search != nullptr)
			enemy[spr.detail].search(plr, i);
	}

	for (i = headspritestat[NUKED]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (spr.picnum == ZFIRE) {
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag <= 0)
				deletesprite(i);
		}
		else {
			if (enemy[spr.detail].nuked != nullptr)
				enemy[spr.detail].nuked(plr, i);
		}
	}

	for (i = headspritestat[FROZEN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		if (enemy[spr.detail].frozen != nullptr)
			enemy[spr.detail].frozen(plr, i);
	}

	for (i = headspritestat[PAIN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].pain != nullptr)
			enemy[spr.detail].pain(plr, i);
	}

	for (i = headspritestat[FACE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].face != nullptr)
			enemy[spr.detail].face(plr, i);
	}

	for (i = headspritestat[ATTACK]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (isWh2() && attacktheme == 0) {
			attacktheme = 1;
			startsong((rand() % 2) + 2);
		}

		if (enemy[spr.detail].attack != nullptr)
			enemy[spr.detail].attack(plr, i);
	}

	for (i = headspritestat[FLEE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].flee != nullptr)
			enemy[spr.detail].flee(plr, i);
	}

	for (i = headspritestat[CAST]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].cast != nullptr)
			enemy[spr.detail].cast(plr, i);
	}

	for (i = headspritestat[DIE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		if (enemy[spr.detail].die != nullptr)
			enemy[spr.detail].die(plr, i);
	}

	for (i = headspritestat[SKIRMISH]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].skirmish != nullptr)
			enemy[spr.detail].skirmish(plr, i);
	}

	for (i = headspritestat[STAND]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		if (enemy[spr.detail].stand != nullptr)
			enemy[spr.detail].stand(plr, i);
	}

	for (i = headspritestat[CHILL]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];
		switch (spr.detail) {
		case GOBLINTYPE:
			goblinChill(plr, i);
			break;
		case SKELETONTYPE:
			skeletonChill(plr, i);
			break;
		}
	}

	for (i = headspritestat[DEAD]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		SPRITE& spr = sprite[i];

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
		switch (checkfluid(i, zr_florhit)) {
		case TYPELAVA:
		case TYPEWATER:
			spr.z = zr_florz + (tileHeight(spr.picnum) << 5);
			break;
		}
	}
}

int aimove(short i) {
	int ox = sprite[i].x;
	int oy = sprite[i].y;
	int oz = sprite[i].z;
	//		short osect = sprite[i].sectnum;

	int movestate = movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
		((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	if (((zr_florz - oz) >> 4) > tileHeight(sprite[i].picnum) + (sprite[i].yrepeat << 2)
		|| (movestate & kHitTypeMask) == kHitWall) {
		//			changespritesect(i, osect);
		//			setsprite(i, ox + mulscale((sprite[i].clipdist) << 2, sintable[(sprite[i].ang + 1536) & 2047], 16),
		//					oy + mulscale((sprite[i].clipdist) << 2, sintable[(sprite[i].ang + 1024) & 2047], 16), oz);

		setsprite(i, ox, oy, oz);

		if ((movestate & kHitTypeMask) != kHitWall) {
			if (isWh2())
				sprite[i].z += WH2GRAVITYCONSTANT;
			else
				sprite[i].z += GRAVITYCONSTANT;
			return 16384 | zr_florhit;
		}
	}

	sprite[i].z = zr_florz;

	return movestate;
}

int aifly(short i) {
	SPRITE& spr = sprite[i];
	int movestate = movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
		((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	spr.z -= TICSPERFRAME << 8;
	short ocs = spr.cstat;
	spr.cstat = 0;
	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, CLIPMASK0);
	spr.cstat = ocs;
	if (spr.z > zr_florz)
		spr.z = zr_florz;
	if (spr.z - (tileHeight(spr.picnum) << 7) < zr_ceilz)
		spr.z = zr_ceilz + (tileHeight(spr.picnum) << 7);

	return movestate;
}

void aisearch(PLAYER& plr, short i, boolean fly) {
	SPRITE& spr = sprite[i];
	spr.lotag -= TICSPERFRAME;

	//		if (plr.invisibletime > 0) {
	//			newstatus(i, FACE);
	//			return;
	//		}

	short osectnum = spr.sectnum;

	int movestat;
	if (fly)
		movestat = aifly(i);
	else
		movestat = aimove(i);

	if (checkdist(plr, i)) {
		if (plr.shadowtime > 0) {
			spr.ang = (short)(((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			newstatus(i, FLEE);
		}
		else
			newstatus(i, ATTACK);
		return;
	}

	if (movestat != 0) {
		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
			spr.sectnum) && spr.lotag < 0) {
			spr.ang = (short)((spr.ang + 1024) & 2047);
			newstatus(i, FLEE);
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
		spr.sectnum) && movestat == 0 && spr.lotag < 0) {
		newstatus(i, FACE);
		return;
	}

	if ((spr.sectnum != osectnum) && (sector[spr.sectnum].lotag == 10))
		warpsprite(i);

	processfluid(i, zr_florhit, fly);

	setsprite(i, spr.x, spr.y, spr.z);
}

boolean checksector6(short i) {
	SPRITE& spr = sprite[i];
	if (sector[spr.sectnum].floorz - (32 << 8) < sector[spr.sectnum].ceilingz) {
		if (sector[spr.sectnum].lotag == 6)
			newstatus(i, DIE);
		else {
			deletesprite(i);
			return true;
		}
	}

	return false;
}

int checkfluid(int i, int zr_florhit) {
	SPRITE& spr = sprite[i];
	if (isValidSector(spr.sectnum) && (zr_florhit & kHitTypeMask) == kHitSector && (sector[spr.sectnum].floorpicnum == WATER
		/* || sector[spr.sectnum].floorpicnum == LAVA2 */ || sector[spr.sectnum].floorpicnum == LAVA
		|| sector[spr.sectnum].floorpicnum == SLIME || sector[spr.sectnum].floorpicnum == FLOORMIRROR
		/*
		 * || sector[spr.sectnum].floorpicnum == LAVA1 ||
		 * sector[spr.sectnum].floorpicnum == ANILAVA
		 */)) {
		if (sector[spr.sectnum].floorpicnum == WATER || sector[spr.sectnum].floorpicnum == SLIME
			|| sector[spr.sectnum].floorpicnum == FLOORMIRROR) {
			return TYPEWATER;
		}
		else {
			return TYPELAVA;
		}
	}

	return TYPENONE;
}

void processfluid(int i, int zr_florhit, boolean fly) {
	SPRITE& spr = sprite[i];
	switch (checkfluid(i, zr_florhit)) {
	case TYPELAVA:
		if (!fly) {
			spr.z += tileHeight(spr.picnum) << 5;
			trailingsmoke(i, true);
			makemonstersplash(LAVASPLASH, i);
		}
		break;
	case TYPEWATER:
		if (!fly) {
			spr.z += tileHeight(spr.picnum) << 5;
			if (krand() % 100 > 60)
				makemonstersplash(SPLASHAROO, i);
		}
		break;
	}
}

void castspell(PLAYER& plr, int i) {
	int j = insertsprite(sprite[i].sectnum, MISSILE);

	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	if (isWh2() || sprite[i].picnum == SPAWNFIREBALL)
		sprite[j].z = sprite[i].z - ((tileHeight(sprite[i].picnum) >> 1) << 8);
	else
		sprite[j].z = getflorzofslope(sprite[i].sectnum, sprite[i].x, sprite[i].y) - ((tileHeight(sprite[i].picnum) >> 1) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = MONSTERBALL;
	sprite[j].shade = -15;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	if (sprite[i].picnum == SPAWNFIREBALL)
		sprite[j].ang = (short)((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + 2048) & 2047);
	else
		sprite[j].ang = (short)(((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + (krand() & 15)
			- 8) + 2048) & 2047);
	sprite[j].xvel = (short)(sintable[(sprite[j].ang + 2560) & 2047] >> 6);
	sprite[j].yvel = (short)(sintable[(sprite[j].ang + 2048) & 2047] >> 6);

	int discrim = ksqrt((plr.x - sprite[j].x) * (plr.x - sprite[j].x) + (plr.y - sprite[j].y) * (plr.y - sprite[j].y));
	if (discrim == 0)
		discrim = 1;
	if (isWh2())
		sprite[j].zvel = (short)(((plr.z + (8 << 8) - sprite[j].z) << 7) / discrim);
	else
		sprite[j].zvel = (short)(((plr.z + (48 << 8) - sprite[j].z) << 7) / discrim);

	sprite[j].owner = (short)i;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 512;
	sprite[j].hitag = 0;
}

void skullycastspell(PLAYER& plr, int i) {
	int j = insertsprite(sprite[i].sectnum, MISSILE);

	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	if (sprite[i].picnum == SPAWNFIREBALL)
		sprite[j].z = sprite[i].z - ((tileHeight(sprite[i].picnum) >> 1) << 8);
	else
		sprite[j].z = getflorzofslope(sprite[i].sectnum, sprite[i].x, sprite[i].y) - ((tileHeight(sprite[i].picnum) >> 1) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = PLASMA;
	sprite[j].shade = -15;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	if (sprite[i].picnum == SPAWNFIREBALL)
		sprite[j].ang = (short)((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + 2048) & 2047);
	else
		sprite[j].ang = (short)(((getangle(plr.x - sprite[j].x, plr.y - sprite[j].y) + (krand() & 15)
			- 8) + 2048) & 2047);
	sprite[j].xvel = (short)(sintable[(sprite[j].ang + 2560) & 2047] >> 6);
	sprite[j].yvel = (short)(sintable[(sprite[j].ang + 2048) & 2047] >> 6);

	int discrim = ksqrt((plr.x - sprite[j].x) * (plr.x - sprite[j].x) + (plr.y - sprite[j].y) * (plr.y - sprite[j].y));
	if (discrim == 0)
		discrim = 1;
	sprite[j].zvel = (short)(((plr.z + (48 << 8) - sprite[j].z) << 7) / discrim);

	sprite[j].owner = (short)i;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 512;
	sprite[j].hitag = 0;
	sprite[j].pal = 7;
}

void attack(PLAYER& plr, int i) {
	int s = 0;
	if (plr.invincibletime > 0 || plr.godMode)
		return;

	if (plr.treasure[TADAMANTINERING] == 1 && (krand() & 1) != 0)
		return;

	//		if ((krand() & (15 < plr.armortype ? 11 : 10)) != 0)
	//			return;

	if (!droptheshield && plr.shieldpoints > 0 && plr.selectedgun > 0 && plr.selectedgun < 5) {
		short a = getangle(sprite[i].x - plr.x, sprite[i].y - plr.y);
		if ((a < plr.ang && plr.ang - a < 128) || (a > plr.ang && (((short)plr.ang + a) & 2047) < 128)) {
			if (krand() % 100 > 80) {
				spritesound(S_SWORD1 + krand() % 3, &sprite[plr.spritenum]);
				return;
			}
			else {
				s = krand() % 50;
				plr.shieldpoints -= s;
				if (krand() % 100 > 50) {
					spritesound(S_SWORD1 + krand() % 3, &sprite[plr.spritenum]);
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

	switch (sprite[i].detail) {
	case SPIDER:
		k = 5;
		break;
	case FISHTYPE:
	case RATTYPE:
		k = 3;
		break;
	case SKELETONTYPE:
		spritesound(S_RIP1 + (krand() % 3), &sprite[i]);
		if ((krand() % 2) != 0)
			spritesound(S_GORE1 + (krand() % 4), &sprite[i]);
		if ((krand() % 2) != 0)
			spritesound(S_BREATH1 + (krand() % 6), &sprite[i]);

		if (isWh2())
			k = (krand() % 5) + 5;
		else
			k >>= 2;
		break;
	case KATIETYPE: // damage 5 - 50
		spritesound(S_DEMONTHROW, &sprite[i]);
		k = (krand() % 45) + 5;
		break;

	case DEVILTYPE:
		spritesound(S_DEMONTHROW, &sprite[i]);
		if (!isWh2())
			k >>= 2;
		break;

	case KOBOLDTYPE:
		spritesound(S_GENSWING, &sprite[i]);
		if ((krand() % 10) > 4) {
			spritesound(S_KOBOLDHIT, &sprite[plr.spritenum]);
			spritesound(S_BREATH1 + (krand() % 6), &sprite[plr.spritenum]);
		}
		if (isWh2())
			k = (krand() % 5) + 5;
		else
			k >>= 2;
		break;
	case FREDTYPE:

		/* Sounds for Fred (currently copied from Goblin) */
		spritesound(S_GENSWING, &sprite[i]);
		if (rand() % 10 > 4)
			spritesound(S_SWORD1 + (rand() % 6), &sprite[i]);

		k >>= 3;
		break;
	case IMPTYPE:
		if (!isWh2())
			break;
		spritesound(S_RIP1 + (krand() % 3), &sprite[i]);
		if ((krand() % 2) != 0) {
			spritesound(S_GORE1 + (krand() % 4), &sprite[i]);
		}
		if ((krand() % 2) != 0) {
			spritesound(S_BREATH1 + (krand() % 6), &sprite[i]);
		}

		k = (krand() % 5) + 5;
		if (k > 8) {
			plr.poisoned = 1;
		}
		break;
	case GOBLINTYPE:
		if (isWh2())
			break;

		spritesound(S_GENSWING, &sprite[i]);
		if ((krand() % 10) > 4)
			spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
		k >>= 2;
		break;
	case NEWGUYTYPE:
		if (sprite[i].picnum == NEWGUYMACE) { // damage 5 - 20
			spritesound(S_PLRWEAPON2, &sprite[i]);
			if (krand() % 10 > 4) {
				spritesound(S_KOBOLDHIT, &sprite[plr.spritenum]);
				spritesound(S_BREATH1 + (krand() % 6), &sprite[plr.spritenum]);
			}
			k = (krand() % 15) + 5;
			break;
		}
	case KURTTYPE:
	case GONZOTYPE:
		spritesound(S_GENSWING, &sprite[i]);
		if (sprite[i].picnum == GONZOCSWAT || sprite[i].picnum == GONZOGSWAT) { // damage 5 - 15
			if (krand() % 10 > 6)
				spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
			k = (krand() % 15) + 5;
		}
		else if (sprite[i].picnum == GONZOGHMAT) { // damage 5 - 15
			if (krand() % 10 > 6)
				spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
			k = (krand() % 10) + 5;
		}
		else if (sprite[i].picnum == GONZOGSHAT) { // damage 5 - 20
			if (krand() % 10 > 3)
				spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
			k = (krand() % 15) + 5;
		}
		else if (sprite[i].picnum == KURTAT) { // damage 5 - 15
			spritesound(S_GENSWING, &sprite[i]);
			if (krand() % 10 > 3) {
				spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
			}
			k = (krand() % 10) + 5;
		}
		else {
			spritesound(S_GENSWING, &sprite[i]);
			if (krand() % 10 > 4) {
				spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
				spritesound(S_BREATH1 + (krand() % 6), &sprite[plr.spritenum]);
			}
			k = (krand() % 4) + 1;
		}
		break;

	case GRONTYPE:
		if (sprite[i].picnum != GRONSWATTACK)
			break;

		if (isWh2()) {
			k = (krand() % 20) + 5;
			if (sprite[i].shade > 30) {
				k += krand() % 10;
			}
		}
		else {
			if (sprite[i].shade > 30)
				k >>= 1;
		}
		spritesound(S_GENSWING, &sprite[i]);
		if ((krand() % 10) > 3)
			spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);

		break;
	case MINOTAURTYPE:
		spritesound(S_GENSWING, &sprite[i]);
		if (krand() % 10 > 4)
			spritesound(S_SWORD1 + (krand() % 6), &sprite[i]);
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

int checkmove(short i, int dax, int day) {
	int movestat = movesprite(i, dax, day, 0, 4 << 8, 4 << 8, CLIFFCLIP);

	if (movestat != 0)
		sprite[i].ang = (short)((sprite[i].ang + TICSPERFRAME) & 2047);

	return movestat;
}

boolean checkdist(PLAYER& plr, int i) {
	if (plr.invisibletime > 0 || plr.health <= 0)
		return false;

	return checkdist(i, plr.x, plr.y, plr.z);
}
	
boolean checkdist(int i, int x, int y, int z) {
	SPRITE& spr = sprite[i];

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

boolean checksight(PLAYER& plr, int i) {
	if (plr.invisibletime > 0) {
		checksight_ang = ((krand() & 512) - 256) & 2047;
		return false;
	}

	if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
		sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum) && plr.invisibletime < 0) {
		checksight_ang = (getangle(plr.x - sprite[i].x, plr.y - sprite[i].y) & 2047);
		if (((sprite[i].ang + 2048 - checksight_ang) & 2047) < 1024)
			sprite[i].ang = (short)((sprite[i].ang + 2048 - (TICSPERFRAME << 1)) & 2047);
		else
			sprite[i].ang = (short)((sprite[i].ang + (TICSPERFRAME << 1)) & 2047);

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

	int j = insertsprite(sprite[i].sectnum, (short)0);

	SPRITE& weap = sprite[j];
	weap.x = sprite[i].x;
	weap.y = sprite[i].y;
	weap.z = sprite[i].z - (24 << 8);
	weap.shade = -15;
	weap.cstat = 0;
	weap.cstat &= ~3;
	weap.pal = 0;

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