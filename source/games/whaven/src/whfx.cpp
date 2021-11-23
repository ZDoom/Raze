#include "ns.h"
#include "wh.h"
#include "gamestate.h"
#include "mapinfo.h"

BEGIN_WH_NS


#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32

#define WATERSIZ 128
#define WATERLOGSIZ 7
#define WATERMAXDROPS 1


int skypanlist[64], skypancnt;
int lavadrylandsector[32];
int lavadrylandcnt;
int bobbingsectorlist[16], bobbingsectorcnt;

DWHActor* lastbat = nullptr;

int revolveclip[16];
int revolvesector[4], revolveang[4], revolvecnt;
int revolvex[4][32], revolvey[4][32];
int revolvepivotx[4], revolvepivoty[4];

static int revolvesyncstat;
static int revolvesyncang, revolvesyncrotang;
static int revolvesyncx, revolvesyncy;

int warpx, warpy, warpz, warpang;
int warpsect;

int scarytime = -1;
int scarysize = 0;

int thunderflash;
int thundertime;


static uint8_t lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2)], lavainc[LAVASIZ];
static int lavanumdrops, lavanumframes;
static int lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
static int lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
static int lavaradx[32][128], lavarady[32][128], lavaradcnt[32];

static uint8_t waterbakpic[(WATERSIZ + 2) * (WATERSIZ + 2)], waterinc[WATERSIZ];
static int waternumdrops, waternumframes;
static int waterdropx[WATERMAXDROPS], waterdropy[WATERMAXDROPS];
static int waterdropsiz[WATERMAXDROPS], waterdropsizlookup[WATERMAXDROPS];
static int waterradx[32][128], waterrady[32][128], waterradcnt[32];

void initlava(void) {

	int x, y, z, r;

	for (x = -16; x <= 16; x++)
		for (y = -16; y <= 16; y++)
		{
			r = ksqrt(x * x + y * y);
			lavaradx[r][lavaradcnt[r]] = x;
			lavarady[r][lavaradcnt[r]] = y;
			lavaradcnt[r]++;
		}

	for (z = 0; z < 16; z++)
		lavadropsizlookup[z] = 8 / (ksqrt(z) + 1);

	for (z = 0; z < LAVASIZ; z++)
		lavainc[z] = abs((((z ^ 17) >> 4) & 7) - 4) + 12;

	lavanumdrops = 0;
	lavanumframes = 0;
}

void movelava(uint8_t* dapic) {

	uint8_t dat, * ptr;
	int x, y, z, zx, dalavadropsiz, dadropsizlookup, offs, offs2;
	int dalavax, dalavay;

	z = 3;
	if (lavanumdrops + z >= LAVAMAXDROPS)
		z = LAVAMAXDROPS - lavanumdrops - 1;
	while (z >= 0)
	{
		lavadropx[lavanumdrops] = (rand() & (LAVASIZ - 1));
		lavadropy[lavanumdrops] = (rand() & (LAVASIZ - 1));
		lavadropsiz[lavanumdrops] = 1;
		lavanumdrops++;
		z--;
	}

	z = lavanumdrops - 1;
	while (z >= 0)
	{
		dadropsizlookup = lavadropsizlookup[lavadropsiz[z]] * (((z & 1) << 1) - 1);
		dalavadropsiz = lavadropsiz[z];
		dalavax = lavadropx[z]; dalavay = lavadropy[z];
		for (zx = lavaradcnt[lavadropsiz[z]] - 1; zx >= 0; zx--)
		{
			offs = (((lavaradx[dalavadropsiz][zx] + dalavax) & (LAVASIZ - 1)) << LAVALOGSIZ);
			offs += ((lavarady[dalavadropsiz][zx] + dalavay) & (LAVASIZ - 1));
			dapic[offs] += dadropsizlookup;
			if (dapic[offs] < 192) dapic[offs] = 192;
		}

		lavadropsiz[z]++;
		if (lavadropsiz[z] > 10)
		{
			lavanumdrops--;
			lavadropx[z] = lavadropx[lavanumdrops];
			lavadropy[z] = lavadropy[lavanumdrops];
			lavadropsiz[z] = lavadropsiz[lavanumdrops];
		}
		z--;
	}

	//Back up dapic with 1 pixel extra on each boundary
	//(to prevent anding for wrap-around)
	auto poffs = (dapic);
	auto poffs2 = (LAVASIZ + 2) + 1 + (lavabakpic);
	for (x = 0; x < LAVASIZ; x++)
	{
		memcpy(poffs2, poffs, LAVASIZ >> 2);
		poffs += LAVASIZ;
		poffs2 += LAVASIZ + 2;
	}
	for (y = 0; y < LAVASIZ; y++)
	{
		lavabakpic[y + 1] = dapic[y + ((LAVASIZ - 1) << LAVALOGSIZ)];
		lavabakpic[y + 1 + (LAVASIZ + 1) * (LAVASIZ + 2)] = dapic[y];
	}
	for (x = 0; x < LAVASIZ; x++)
	{
		lavabakpic[(x + 1) * (LAVASIZ + 2)] = dapic[(x << LAVALOGSIZ) + (LAVASIZ - 1)];
		lavabakpic[(x + 1) * (LAVASIZ + 2) + (LAVASIZ + 1)] = dapic[x << LAVALOGSIZ];
	}
	lavabakpic[0] = dapic[LAVASIZ * LAVASIZ - 1];
	lavabakpic[LAVASIZ + 1] = dapic[LAVASIZ * (LAVASIZ - 1)];
	lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 1)] = dapic[LAVASIZ - 1];
	lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2) - 1] = dapic[0];

	for (z = (LAVASIZ + 2) * (LAVASIZ + 2) - 4; z >= 0; z -= 4) {
		lavabakpic[z + 0] &= 31;
		lavabakpic[z + 1] &= 31;
		lavabakpic[z + 2] &= 31;
		lavabakpic[z + 3] &= 31;
	}


	for (x = LAVASIZ - 1; x >= 0; x--)
	{
		offs = (x + 1) * (LAVASIZ + 2) + 1;
		ptr = (uint8_t*)((x << LAVALOGSIZ) + dapic);

		zx = ((x + lavanumframes) & (LAVASIZ - 1));

		offs2 = LAVASIZ - 1;
		for (y = offs; y < offs + LAVASIZ; y++)
		{
			dat = lavainc[(offs2--) & zx];
			dat += lavabakpic[y - (LAVASIZ + 2) - 1];
			dat += lavabakpic[y - (LAVASIZ + 2)];
			dat += lavabakpic[y - (LAVASIZ + 2) + 1];
			dat += lavabakpic[y - 1];
			dat += lavabakpic[y + 1];
			dat += lavabakpic[y + (LAVASIZ + 2)];
			dat += lavabakpic[y + (LAVASIZ + 2) - 1];
			*ptr++ = (dat >> 3) + 192;//was 192
		}
	}

	lavanumframes++;
}

void initwater(void) {

	int x, y, z, r;

	for (x = -16; x <= 16; x++)
		for (y = -16; y <= 16; y++)
		{
			r = ksqrt(x * x + y * y);
			waterradx[r][waterradcnt[r]] = x;
			waterrady[r][waterradcnt[r]] = y;
			waterradcnt[r]++;
		}

	for (z = 0; z < 16; z++)
		waterdropsizlookup[z] = 8 / (ksqrt(z) + 1);

	for (z = 0; z < WATERSIZ; z++)
		waterinc[z] = abs((((z ^ 17) >> 4) & 7) - 4) + 12;

	waternumdrops = 0;
	waternumframes = 0;
}

void movewater(uint8_t* dapic) {

	uint8_t dat, * ptr;
	int x, y, z, zx, dawaterdropsiz, dadropsizlookup, offs, offs2;
	int dawaterx, dawatery;

	z = 3;
	if (waternumdrops + z >= WATERMAXDROPS)
		z = WATERMAXDROPS - waternumdrops - 1;
	while (z >= 0)
	{
		waterdropx[waternumdrops] = (rand() & (WATERSIZ - 1));
		waterdropy[waternumdrops] = (rand() & (WATERSIZ - 1));
		waterdropsiz[waternumdrops] = 1;
		waternumdrops++;
		z--;
	}
	z = waternumdrops - 1;
	while (z >= 0)
	{
		dadropsizlookup = waterdropsizlookup[waterdropsiz[z]] * (((z & 1) << 1) - 1);
		dawaterdropsiz = waterdropsiz[z];
		dawaterx = waterdropx[z]; dawatery = waterdropy[z];
		for (zx = waterradcnt[waterdropsiz[z]] - 1; zx >= 0; zx--)
		{
			offs = (((waterradx[dawaterdropsiz][zx] + dawaterx) & (WATERSIZ - 1)) << WATERLOGSIZ);
			offs += ((waterrady[dawaterdropsiz][zx] + dawatery) & (WATERSIZ - 1));
			dapic[offs] += dadropsizlookup;
			if (dapic[offs] < 224) dapic[offs] = 224;
		}

		waterdropsiz[z]++;
		if (waterdropsiz[z] > 10)
		{
			waternumdrops--;
			waterdropx[z] = waterdropx[waternumdrops];
			waterdropy[z] = waterdropy[waternumdrops];
			waterdropsiz[z] = waterdropsiz[waternumdrops];
		}
		z--;
	}

	auto poffs = (dapic);
	auto poffs2 = (WATERSIZ + 2) + 1 + (waterbakpic);
	for (x = 0; x < WATERSIZ; x++)
	{
		memcpy(poffs2, poffs, WATERSIZ);
		poffs += WATERSIZ;
		poffs2 += WATERSIZ + 2;
	}
	for (y = 0; y < WATERSIZ; y++)
	{
		waterbakpic[y + 1] = dapic[y + ((WATERSIZ - 1) << WATERLOGSIZ)];
		waterbakpic[y + 1 + (WATERSIZ + 1) * (WATERSIZ + 2)] = dapic[y];
	}
	for (x = 0; x < WATERSIZ; x++)
	{
		waterbakpic[(x + 1) * (WATERSIZ + 2)] = dapic[(x << WATERLOGSIZ) + (WATERSIZ - 1)];
		waterbakpic[(x + 1) * (WATERSIZ + 2) + (WATERSIZ + 1)] = dapic[x << WATERLOGSIZ];
	}
	waterbakpic[0] = dapic[WATERSIZ * WATERSIZ - 1];
	waterbakpic[WATERSIZ + 1] = dapic[WATERSIZ * (WATERSIZ - 1)];
	waterbakpic[(WATERSIZ + 2) * (WATERSIZ + 1)] = dapic[WATERSIZ - 1];
	waterbakpic[(WATERSIZ + 2) * (WATERSIZ + 2) - 1] = dapic[0];

	for (z = (WATERSIZ + 2) * (WATERSIZ + 2) - 4; z >= 0; z -= 4) {
		waterbakpic[z + 0] &= 15;
		waterbakpic[z + 1] &= 15;
		waterbakpic[z + 2] &= 15;
		waterbakpic[z + 3] &= 15;
	}


	for (x = WATERSIZ - 1; x >= 0; x--)
	{
		offs = (x + 1) * (WATERSIZ + 2) + 1;
		ptr = (uint8_t*)((x << WATERLOGSIZ) + dapic);

		zx = ((x + waternumframes) & (WATERSIZ - 1));

		offs2 = WATERSIZ - 1;
		for (y = offs; y < offs + WATERSIZ; y++)
		{
			dat = waterinc[(offs2--) & zx];
			dat += waterbakpic[y - (WATERSIZ + 2) - 1];
			dat += waterbakpic[y - (WATERSIZ + 2)];
			dat += waterbakpic[y - (WATERSIZ + 2) + 1];
			dat += waterbakpic[y - 1];
			dat += waterbakpic[y + 1];
			dat += waterbakpic[y + (WATERSIZ + 2) + 1];
			dat += waterbakpic[y + (WATERSIZ + 2)];
			dat += waterbakpic[y + (WATERSIZ + 2) - 1];
			*ptr++ = (dat >> 3) + 223;
		}
	}

	waternumframes++;
}

void skypanfx() {
	for (int i = 0; i < skypancnt; i++) {
		sector[skypanlist[i]].setceilingxpan(-float(PlayClock & 255));
	}
}

void panningfx() {
	for (int i = 0; i < floorpanningcnt; i++) {
		int whichdir = sector[floorpanninglist[i]].lotag - 80;

		switch (whichdir) {
		case 0:
			sector[floorpanninglist[i]].setfloorypan(float(PlayClock & 255));
			break;
		case 1:
			sector[floorpanninglist[i]].setfloorxpan(-float(PlayClock & 255));
			sector[floorpanninglist[i]].setfloorypan(float(PlayClock & 255));
			break;
		case 2:
			sector[floorpanninglist[i]].setfloorxpan(-float(PlayClock & 255));
			break;
		case 3:
			sector[floorpanninglist[i]].setfloorxpan(-float(PlayClock & 255));
			sector[floorpanninglist[i]].setfloorypan(-float(PlayClock & 255));
			break;
		case 4:
			sector[floorpanninglist[i]].setfloorypan(-float(PlayClock & 255));
			break;
		case 5:
			sector[floorpanninglist[i]].setfloorxpan(float(PlayClock & 255));
			sector[floorpanninglist[i]].setfloorypan(-float(PlayClock & 255));
			break;
		case 6:
			sector[floorpanninglist[i]].setfloorxpan(float(PlayClock & 255));
			break;
		case 7:
			sector[floorpanninglist[i]].setfloorxpan(float(PlayClock & 255));
			sector[floorpanninglist[i]].setfloorypan(float(PlayClock & 255));
			break;
		default:
			sector[floorpanninglist[i]].setfloorxpan(0);
			sector[floorpanninglist[i]].setfloorypan(0);
			break;
		}
	}

	for (int i = 0; i < xpanningsectorcnt; i++) {
		int dasector = xpanningsectorlist[i];
		int startwall = sector[dasector].wallptr;
		int endwall = startwall + sector[dasector].wallnum - 1;
		for (int s = startwall; s <= endwall; s++)
			wall[s].setxpan(float(PlayClock & 255));
	}

	for (int i = 0; i < ypanningwallcnt; i++)
		wall[ypanningwalllist[i]].setypan((float)~(PlayClock & 255));
}

void revolvefx() {

	int startwall, endwall;

	int dax, day;
	PLAYER& plr = player[pyrn];

	for (int i = 0; i < revolvecnt; i++) {

		startwall = sector[revolvesector[i]].wallptr;
		endwall =  (startwall + sector[revolvesector[i]].wallnum - 1);

		revolveang[i] =  ((revolveang[i] + 2048 - ((TICSPERFRAME) << 1)) & 2047);
		for (int k = startwall; k <= endwall; k++) {
			Point out = rotatepoint(revolvepivotx[i], revolvepivoty[i], revolvex[i][k - startwall],
					revolvey[i][k - startwall], revolveang[i]);
			dax = out.getX();
			day = out.getY();
			dragpoint(k, dax, day);
		}

		if (plr.sector == revolvesector[i]) {
			revolvesyncang = plr.angle.ang.asbuild();
			revolvesyncrotang = 0;
			revolvesyncx = plr.x;
			revolvesyncy = plr.y;
			revolvesyncrotang =  ((revolvesyncrotang + 2048 - ((TICSPERFRAME) << 1)) & 2047);
			Point out = rotatepoint(revolvepivotx[i], revolvepivoty[i], revolvesyncx, revolvesyncy,
					revolvesyncrotang);
			viewBackupPlayerLoc(pyrn);
			plr.x = out.getX();
			plr.y = out.getY();
			plr.angle.settarget((revolvesyncang + revolvesyncrotang) & 2047);
		}
	}
}

void bobbingsector() {
	for (int i = 0; i < bobbingsectorcnt; i++) {
		int dasector = bobbingsectorlist[i];
		sector[dasector].floorz += bsin(PlayClock << 4, -6);
	}
}

void teleporter() {

	int dasector;
	int startwall, endwall;
	int i, j;
	int s;
	int16_t daang;

	auto &plr = player[pyrn];

	for (i = 0; i < warpsectorcnt; i++) {
		dasector = warpsectorlist[i];
		j = ((PlayClock & 127) >> 2);
		if (j >= 16)
			j = 31 - j;
		{
			sector[dasector].ceilingshade = (byte) j;
			sector[dasector].floorshade = (byte) j;
			startwall = sector[dasector].wallptr;
			endwall =  (startwall + sector[dasector].wallnum - 1);
			for (s = startwall; s <= endwall; s++)
				wall[s].shade = (byte) j;
		}
	}
	if (plr.sector == -1)
		return;

	if (plr.Sector()->lotag == 10) {
		if (plr.sector != plr.oldsector) {
			daang = plr.angle.ang.asbuild();
			warpfxsprite(plr.actor());
			warp(plr.x, plr.y, plr.z, daang, plr.sector);
			viewBackupPlayerLoc(pyrn);
			plr.x = warpx;
			plr.y = warpy;
			plr.z = warpz;
			daang =  warpang;
			plr.sector =  warpsect;
			warpfxsprite(plr.actor());
			plr.angle.settarget(daang);
			plr.justwarpedfx = 48;
			spritesound(S_WARP, plr.actor());
			SetActorPos(plr.actor(), plr.x, plr.y, plr.z + (32 << 8));
		}
	}

	if (plr.Sector()->lotag == 4002) {
		if (plr.sector != plr.oldsector) {
			if (plr.treasure[TPENTAGRAM] == 1) {
				plr.treasure[TPENTAGRAM] = 0;
#pragma message ("usermap")
#if 0
				if (mUserFlag == UserFlag.UserMap) {
					game.changeScreen(gMenuScreen);
					return;
				}
#endif
				switch (plr.Sector()->hitag) {
				case 1: // NEXTLEVEL
					justteleported = true;
					CompleteLevel(currentLevel);
					break;
				case 2: // ENDOFDEMO
					spritesound(S_THUNDER1, plr.actor());
					justteleported = true;
					CompleteLevel(nullptr);
					break;
				}
			} else {
				// player need pentagram to teleport
				showmessage("ITEM NEEDED", 360);
			}
		}
	}
}

void warp(int x, int y, int z, int daang, int dasector) {
	warpx = x;
	warpy = y;
	warpz = z;
	warpang = daang;
	warpsect = dasector;

	for (int i = 0; i < warpsectorcnt; i++) {
		if (sector[warpsectorlist[i]].hitag == sector[warpsect].hitag && warpsectorlist[i] != warpsect) {
			warpsect = warpsectorlist[i];
			break;
		}
	}

	// find center of sector
	int startwall = sector[warpsect].wallptr;
	int endwall =  (startwall + sector[warpsect].wallnum - 1);
	int dax = 0, day = 0, i = 0;
	for (int s = startwall; s <= endwall; s++) {
		dax += wall[s].x;
		day += wall[s].y;
		if (wall[s].twoSided()) {
			i = s;
		}
	}
	warpx = dax / (endwall - startwall + 1);
	warpy = day / (endwall - startwall + 1);
	warpz = sector[warpsect].floorz - (32 << 8);

	updatesector(warpx, warpy, &warpsect);
	dax = ((wall[i].x + wall[wall[i].point2].x) >> 1);
	day = ((wall[i].y + wall[wall[i].point2].y) >> 1);
	warpang = getangle(dax - warpx, day - warpy);
}

void warpsprite(DWHActor* actor) {
	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	if (monsterwarptime > 0)
		return;
	auto& spr = actor->s();
	int dasectnum = spr.sectnum;
	warpfxsprite(actor);
	warp(spr.x, spr.y, spr.z, spr.ang, dasectnum);
	spr.x = warpx;
	spr.y = warpy;
	spr.z = warpz;
	spr.ang =  warpang;
	dasectnum =  warpsect;

	warpfxsprite(actor);
	SetActorPos(actor, spr.x, spr.y, spr.z);

	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	monsterwarptime = 120;
}

void ironbars() {
	for (int i = 0; i < ironbarscnt; i++) {
		if (ironbarsdone[i] == 1) {
			auto actor = ironbarsanim[i];
			auto& spr = actor->s();
			switch (spr.hitag) {
			case 1:
				spr.ang += TICSPERFRAME << 1;
				if (spr.ang > 2047)
					spr.ang -= 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				SetActorPos(actor, &spr.pos);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					spr.hitag = 2;
					ironbarsdone[i] = 0;
				}
				break;
			case 2:
				spr.ang -= TICSPERFRAME << 1;
				if (spr.ang < 0)
					spr.ang += 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				SetActorPos(actor, &spr.pos);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					spr.hitag = 1;
					ironbarsdone[i] = 0;
				}
				break;
			case 3:
				spr.z -= TICSPERFRAME << 4;
				if (spr.z < ironbarsgoal[i]) {
					spr.z = ironbarsgoal[i];
					spr.hitag = 4;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = spr.z + 6000;
				}
				SetActorPos(actor, &spr.pos);
				break;
			case 4:
				spr.z += TICSPERFRAME << 4;
				if (spr.z > ironbarsgoal[i]) {
					spr.z = ironbarsgoal[i];
					spr.hitag = 3;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = spr.z - 6000;
				}
				SetActorPos(actor, &spr.pos);
				break;
			}
		}
	}
}

void sectorsounds() {
	if (!SoundEnabled())
		return;

	PLAYER& plr = player[pyrn];

	int sec = plr.Sector()->extra & 0xFFFF;
	if (sec != 0) 
	{
		if ((sec & 32768) != 0) 
		{ // loop on/off sector
			if ((sec & 1) != 0) 
			{ // turn loop on if lsb is 1
				int index = (sec & ~0x8001) >> 1;
				if (index < MAX_AMB_SOUNDS && soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, 0, CHAN_AMBIENT1 + index) == 0)
				{
					playsound(ambsoundarray[index], 0, 0, 0, CHAN_AMBIENT1 + index);
				}
			} else { // turn loop off if lsb is 0 and its playing
				int index = (sec & ~0x8000) >> 1;
				soundEngine->StopSound(CHAN_AMBIENT1 + index);
			}
		} else {
			if (plr.z <= plr.Sector()->floorz - (8 << 8))
				spritesound(sec, plr.actor());
		}
	}
}


void scaryprocess() {
	if (krand() % 32768 > 32500 && krand() % 32768 > 32500 && scarytime < 0) {
		scarytime = 180;
		scarysize = 30;
		SND_Sound(S_SCARYDUDE);
	}

	if (scarytime >= 0) {
		scarytime -= TICSPERFRAME << 1;
		scarysize += TICSPERFRAME << 1;
	}
}

void dofx() {
	lavadryland();
	scaryprocess();
	if (revolvecnt > 0)
		revolvefx();
	panningfx();
	teleporter();
	bobbingsector();
	if (ironbarscnt > 0)
		ironbars();

	if ((gotpic[ANILAVA >> 3] & (1 << (ANILAVA & 7))) > 0) {
		gotpic[ANILAVA >> 3] &= ~(1 << (ANILAVA & 7));
		movelava(tileData(ANILAVA));
	}
	if ((gotpic[HEALTHWATER >> 3] & (1 << (HEALTHWATER & 7))) > 0) {
		gotpic[HEALTHWATER >> 3] &= ~(1 << (HEALTHWATER & 7));
		movelava(tileData(HEALTHWATER));
	}
	thesplash();
	thunder();
	cracks();
	if (isWh2()) {
		PLAYER& plr = player[0];
		if (plr.Sector()->lotag == 50 && plr.Sector()->hitag > 0)
			weaponpowerup(plr);
	}

#pragma message ("drunk?")
#if 0
	GLRenderer gl = glrender();
	if (gl != nullptr) {
		if (player[pyrn].poisoned != 0) {
			int tilt = MulScale(bsin(3 * PlayClock), 20, 16);
			if (tilt != 0)
				gl.setdrunk(tilt);
		} else
			gl.setdrunk(0);
	}
#endif
}

void thunder() {
	int val;

	if (thunderflash == 0) {
		g_visibility = 1024;
		if ((gotpic[SKY >> 3] & (1 << (SKY & 7))) > 0) {
			gotpic[SKY >> 3] &= ~(1 << (SKY & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY2 >> 3] & (1 << (SKY2 & 7))) > 0) {
			gotpic[SKY2 >> 3] &= ~(1 << (SKY2 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY3 >> 3] & (1 << (SKY3 & 7))) > 0) {
			gotpic[SKY3 >> 3] &= ~(1 << (SKY3 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY4 >> 3] & (1 << (SKY4 & 7))) > 0) {
			gotpic[SKY4 >> 3] &= ~(1 << (SKY4 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY5 >> 3] & (1 << (SKY5 & 7))) > 0) {
			gotpic[SKY5 >> 3] &= ~(1 << (SKY5 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY6 >> 3] & (1 << (SKY6 & 7))) > 0) {
			gotpic[SKY6 >> 3] &= ~(1 << (SKY6 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY7 >> 3] & (1 << (SKY7 & 7))) > 0) {
			gotpic[SKY7 >> 3] &= ~(1 << (SKY7 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY8 >> 3] & (1 << (SKY8 & 7))) > 0) {
			gotpic[SKY8 >> 3] &= ~(1 << (SKY8 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY9 >> 3] & (1 << (SKY9 & 7))) > 0) {
			gotpic[SKY9 >> 3] &= ~(1 << (SKY9 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY10 >> 3] & (1 << (SKY10 & 7))) > 0) {
			gotpic[SKY10 >> 3] &= ~(1 << (SKY10 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		}
	} else {
		thundertime -= TICSPERFRAME;
		if (thundertime < 0) {
			thunderflash = 0;
			SND_Sound(S_THUNDER1 + (krand() % 4));
			g_visibility = 1024;
		}
	}

	if (thunderflash == 1) {
		val = krand() % 4;
		switch (val) {
		case 0:
			g_visibility = 2048;
			break;
		case 1:
			g_visibility = 1024;
			break;
		case 2:
			g_visibility = 512;
			break;
		case 3:
			g_visibility = 256;
			break;
		default:
			g_visibility = 4096;
			break;
		}
	}
}

void thesplash() {
	PLAYER& plr = player[pyrn];

	if (plr.sector == -1)
		return;

	if (plr.Sector()->floorpicnum == WATER || plr.Sector()->floorpicnum == LAVA
			|| plr.Sector()->floorpicnum == SLIME) {
		if (plr.onsomething == 0)
			return;

		if (plr.sector != plr.oldsector) {
			switch (plr.Sector()->floorpicnum) {
			case WATER:
				makeasplash(SPLASHAROO, plr);
				break;
			case SLIME:
				if (isWh2()) {
					makeasplash(SLIMESPLASH, plr);
				} else makeasplash(SPLASHAROO, plr);
				break;
			case LAVA:
				makeasplash(LAVASPLASH, plr);
				break;
			}
		}
	}
}

void makeasplash(int picnum, PLAYER& plr) {
	auto spawnedactor = InsertActor(plr.sector, MASPLASH);
	auto& spawned = spawnedactor->s();

	spawned.x = plr.x;
	spawned.y = plr.y;
	spawned.z = plr.Sector()->floorz + (tileHeight(picnum) << 8);
	spawned.cstat = 0; // Hitscan does not hit other bullets
	spawned.picnum =  picnum;
	spawned.shade = 0;
	spawned.pal = 0;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawnedactor->SetOwner(nullptr);
	spawned.clipdist = 16;
	spawned.lotag = 8;
	spawned.hitag = 0;

	switch (picnum) {
	case SPLASHAROO:
	case SLIMESPLASH:
		if(!isWh2() && picnum == SLIMESPLASH)
			break;
			
		spritesound(S_SPLASH1 + (krand() % 3), spawnedactor);
		break;
	case LAVASPLASH:
		break;
	}

	movesprite(spawnedactor, (bcos(spawned.ang) * TICSPERFRAME) << 3,
			(bsin(spawned.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
	spawned.backuploc();
}

void makemonstersplash(int picnum, DWHActor* actor) {
	auto& spr = actor->s();
	if (spr.picnum == FISH)
		return;

	auto spawnedactor = InsertActor(spr.sectnum, MASPLASH);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = sector[spr.sectnum].floorz + (tileHeight(picnum) << 8);
	spawned.cstat = 0; // Hitscan does not hit other bullets
	spawned.picnum =  picnum;
	spawned.shade = 0;

	if (sector[spr.sectnum].floorpal == 9)
		spawned.pal = 9;
	else
		spawned.pal = 0;
		
	if (spr.picnum == RAT) {
		spawned.xrepeat = 40;
		spawned.yrepeat = 40;
	} else {
		spawned.xrepeat = 64;
		spawned.yrepeat = 64;
	}
	spawnedactor->SetOwner(nullptr);
	spawned.clipdist = 16;
	spawned.lotag = 8;
	spawned.hitag = 0;
	spawned.backuploc();

	// JSA 5/3 start
	switch (picnum) {
	case SPLASHAROO:
	case SLIME:
		if ((krand() % 2) != 0) {
			if ((gotpic[WATER >> 3] & (1 << (WATER & 7))) > 0) {
				gotpic[WATER >> 3] &= ~(1 << (WATER & 7));
					if ((krand() % 2) != 0)
						spritesound(S_SPLASH1 + (krand() % 3), spawnedactor);
			}
		}
		if ((krand() % 2) != 0) {
			if ((gotpic[SLIME >> 3] & (1 << (SLIME & 7))) > 0) {
				gotpic[SLIME >> 3] &= ~(1 << (SLIME & 7));
					if ((krand() % 2) != 0)
						spritesound(S_SPLASH1 + (krand() % 3), spawnedactor);
			}
		}
		break;
	case LAVASPLASH:
		break;
	}
}

void bats(PLAYER& plr, DWHActor* actor) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, FLOCK);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z;
	spawned.cstat = 0;
	spawned.picnum = BAT;
	spawned.shade = 0;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawned.ang =  ((spr.ang + (krand() & 128 - 256)) & 2047);
	spawnedactor->SetOwner(actor);
	spawned.clipdist = 16;
	spawned.lotag = 128;
	//spawned.hitag =  k; // see: flying in circles
	spawned.extra = 0;
	spawned.backuploc();

	SetNewStatus(spawnedactor, FLOCK);

	if (spr.extra == 1)
		lastbat = spawnedactor;
}

void cracks() {

	PLAYER& plr = player[0];
	if (plr.sector == -1)
		return;

	int datag = plr.Sector()->lotag;

	if (floorpanningcnt < 64)
		if (datag >= 3500 && datag <= 3599) {
			plr.Sector()->hitag = 0;
			int daz = plr.Sector()->floorz + (1024 * (plr.Sector()->lotag - 3500));
			if ((setanimation(plr.sector, daz, 32, 0, FLOORZ)) >= 0) {
				plr.Sector()->floorpicnum = LAVA1;
				plr.Sector()->floorshade = -25;
				SND_Sound(S_CRACKING);
			}
			plr.Sector()->lotag = 80;
			floorpanninglist[floorpanningcnt++] = plr.sector;
		}

	if (datag >= 5100 && datag <= 5199) {
		plr.Sector()->hitag = 0;
		plr.Sector()->lotag = 0;
	}

	if (datag >= 5200 && datag <= 5299) {
		plr.Sector()->hitag = 0;
		plr.Sector()->lotag = 0;
	}

	if (datag == 3001) {
		plr.Sector()->lotag = 0;
		WHSpriteIterator it;
		while (auto itActor = it.Next())
		{
			auto& spk = itActor->s();
			if (plr.Sector()->hitag == spk.hitag) {
				spk.lotag = 36;
				spk.zvel =  (krand() & 1024 + 512);
				SetNewStatus(itActor, SHOVE);
			}
		}
	}
}

void lavadryland() {
	PLAYER& plr = player[pyrn];

	for (int k = 0; k < lavadrylandcnt; k++) {

		int s = lavadrylandsector[k];

		if (plr.sector == s && sector[s].lotag > 0) {

			sector[s].hitag = 0;

			switch (sector[s].floorpicnum) {
			case LAVA:
			case ANILAVA:
			case LAVA1:
				sector[s].floorpicnum = COOLLAVA;
				break;
			case SLIME:
				sector[s].floorpicnum = DRYSLIME;
				break;
			case WATER:
			case HEALTHWATER:
				sector[s].floorpicnum = DRYWATER;
				break;
			case LAVA2:
				sector[s].floorpicnum = COOLLAVA2;
				break;
			}
			sector[s].lotag = 0;
		}
	}

}

void warpfxsprite(DWHActor* actor) {
	auto& spr = actor->s();
	PLAYER& plr = player[pyrn];

	auto spawnedactor = InsertActor(spr.sectnum, WARPFX);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z - (32 << 8);

	spawned.cstat = 0;

	spawned.picnum = ANNIHILATE;
	int16_t daang;
	if (actor == plr.actor()) {
		daang = plr.angle.ang.asbuild();
		spawned.ang = daang;
	} else {
		daang = spr.ang;
		spawned.ang = daang;
	}

	spawned.xrepeat = 48;
	spawned.yrepeat = 48;
	spawned.clipdist = 16;

	spawned.extra = 0;
	spawned.shade = -31;
	spawned.xvel =  ((krand() & 256) - 128);
	spawned.yvel =  ((krand() & 256) - 128);
	spawned.zvel =  ((krand() & 256) - 128);
	spawnedactor->SetOwner(actor);
	spawned.lotag = 12;
	spawned.hitag = 0;
	spawned.pal = 0;

	int daz = (((spawned.zvel) * TICSPERFRAME) >> 3);

	movesprite(spawnedactor, (bcos(daang) * TICSPERFRAME) << 3,
			(bsin(daang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);
	spawned.backuploc();
}

void resetEffects() {
#if 0
	greencount = 0;
	bluecount = 0;
	redcount = 0;
	whitecount = 0;
	updateFade("RED", 0);
	updateFade("GREEN", 0);
	updateFade("BLUE", 0);
	updateFade("WHITE", 0);
#endif
}

void weaponpowerup(PLAYER& plr) {
	showmessage("Weapons enchanted", 360);
	for (int i = 0; i < 10; i++) {
		if (plr.weapon[i] != 0 && plr.weapon[i] != 3) {
			plr.preenchantedweapon[i] = plr.weapon[i];
			plr.preenchantedammo[i] = plr.ammo[i];
			plr.weapon[i] = 3;
			switch (difficulty) {
			case 0:
				plr.ammo[i] = 25;
				break;
			case 1:
				plr.ammo[i] = 20;
				break;
			case 2:
			case 3:
				plr.ammo[i] = 10;
				break;
			}
				
			if (plr.Sector()->hitag > 0) {
				plr.Sector()->hitag--;
				if (plr.Sector()->hitag == 0) {
					WHSectIterator it(plr.sector);
					while (auto actor = it.Next())
					{
						SPRITE& tspr = actor->s();

						if (tspr.picnum == CONE) {
							DeleteActor(actor);
						} else if (tspr.picnum == SPARKBALL) {
							DeleteActor(actor);
						}
					}
				}
			}
		}
	}
}

void makesparks(DWHActor* actor, int type) {

	auto& spr = actor->s();
	DWHActor* spawnedactor = nullptr;

	switch (type) {
	case 1:
		spawnedactor = InsertActor(spr.sectnum, SPARKS);
		break;
	case 2:
		spawnedactor = InsertActor(spr.sectnum, SPARKSUP);
		break;
	case 3:
		spawnedactor = InsertActor(spr.sectnum, SPARKSDN);
		break;
	}

	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z;
	spawned.cstat = 0;
	spawned.picnum = SPARKBALL;
	spawned.shade = 0;
	spawned.xrepeat = 24;
	spawned.yrepeat = 24;
	spawned.ang =  ((krand() % 2047) & 2047);
	spawnedactor->SetOwner(nullptr);
	spawned.clipdist = 16;
	spawned.lotag =  (krand() % 100);
	spawned.hitag = 0;
	spawned.extra = 0;

	spawned.pal = 0;
	spawned.backuploc();
}

void shards(DWHActor* actor, int type) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, SHARDOFGLASS);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x + (((krand() % 512) - 256) << 2);
	spawned.y = spr.y + (((krand() % 512) - 256) << 2);
	spawned.z = spr.z - (getPlayerHeight() << 8) + (((krand() % 48) - 16) << 7);
	spawned.zvel =  (krand() % 256);
	spawned.cstat = 0;
	spawned.picnum =  (SHARD + (krand() % 3));
	spawned.shade = 0;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawned.ang =  (spr.ang + ((krand() % 512) - 256) & 2047);
	spawnedactor->SetOwner(actor);
	spawned.clipdist = 16;
	spawned.lotag =  (120 + (krand() % 100));
	spawned.hitag = 0;
	spawned.extra =  type;
	spawned.pal = 0;
	spawned.backuploc();
}


END_WH_NS
