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


short skypanlist[64], skypancnt;
short lavadrylandsector[32];
short lavadrylandcnt;
short bobbingsectorlist[16], bobbingsectorcnt;

int justwarpedfx = 0;
int lastbat = -1;

short revolveclip[16];
short revolvesector[4], revolveang[4], revolvecnt;
int revolvex[4][32], revolvey[4][32];
int revolvepivotx[4], revolvepivoty[4];

static int revolvesyncstat;
static short revolvesyncang, revolvesyncrotang;
static int revolvesyncx, revolvesyncy;

int warpx, warpy, warpz, warpang;
short warpsect;

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
		lavainc[z] = klabs((((z ^ 17) >> 4) & 7) - 4) + 12;

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
		waterinc[z] = klabs((((z ^ 17) >> 4) & 7) - 4) + 12;

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
		sector[skypanlist[i]].ceilingxpanning = (short) -((lockclock >> 2) & 255);
	}
}

void panningfx() {
	for (int i = 0; i < floorpanningcnt; i++) {
		int whichdir = sector[floorpanninglist[i]].lotag - 80;

		switch (whichdir) {
		case 0:
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		case 1:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		case 2:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			break;
		case 3:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 4:
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 5:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 6:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			break;
		case 7:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		default:
			sector[floorpanninglist[i]].floorxpanning = 0;
			sector[floorpanninglist[i]].floorypanning = 0;
			break;
		}
	}

	for (int i = 0; i < xpanningsectorcnt; i++) {
		int dasector = xpanningsectorlist[i];
		int startwall = sector[dasector].wallptr;
		int endwall = startwall + sector[dasector].wallnum - 1;
		for (int s = startwall; s <= endwall; s++)
			wall[s].xpanning = (short) ((lockclock >> 2) & 255);
	}

	for (int i = 0; i < ypanningwallcnt; i++)
		wall[ypanningwalllist[i]].ypanning = (short) ~(lockclock & 255);
}

void revolvefx() {

	short startwall, endwall;

	int dax, day;
	PLAYER& plr = player[pyrn];

	for (int i = 0; i < revolvecnt; i++) {

		startwall = sector[revolvesector[i]].wallptr;
		endwall = (short) (startwall + sector[revolvesector[i]].wallnum - 1);

		revolveang[i] = (short) ((revolveang[i] + 2048 - ((TICSPERFRAME) << 1)) & 2047);
		for (short k = startwall; k <= endwall; k++) {
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
			revolvesyncrotang = (short) ((revolvesyncrotang + 2048 - ((TICSPERFRAME) << 1)) & 2047);
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
		short dasector = bobbingsectorlist[i];
		sector[dasector].floorz += (sintable[(lockclock << 4) & 2047] >> 6);
	}
}

void teleporter() {

	short dasector;
	short startwall, endwall;
	int i, j;
	int s;
	short daang;

	auto &plr = player[pyrn];

	for (i = 0; i < warpsectorcnt; i++) {
		dasector = warpsectorlist[i];
		j = ((lockclock & 127) >> 2);
		if (j >= 16)
			j = 31 - j;
		{
			sector[dasector].ceilingshade = (byte) j;
			sector[dasector].floorshade = (byte) j;
			startwall = sector[dasector].wallptr;
			endwall = (short) (startwall + sector[dasector].wallnum - 1);
			for (s = startwall; s <= endwall; s++)
				wall[s].shade = (byte) j;
		}
	}
	if (plr.sector == -1)
		return;

	if (sector[plr.sector].lotag == 10) {
		if (plr.sector != plr.oldsector) {
			daang = plr.angle.ang.asbuild();
			warpfxsprite(plr.spritenum);
			warp(plr.x, plr.y, plr.z, daang, plr.sector);
			viewBackupPlayerLoc(pyrn);
			plr.x = warpx;
			plr.y = warpy;
			plr.z = warpz;
			daang = (short) warpang;
			plr.sector = (short) warpsect;
			warpfxsprite(plr.spritenum);
			plr.angle.settarget(daang);
			justwarpedfx = 48;
			spritesound(S_WARP, &sprite[plr.spritenum]);
			setsprite(plr.spritenum, plr.x, plr.y, plr.z + (32 << 8));
		}
	}

	if (sector[plr.sector].lotag == 4002) {
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
				switch (sector[plr.sector].hitag) {
				case 1: // NEXTLEVEL
					justteleported = true;
					CompleteLevel(currentLevel);
					break;
				case 2: // ENDOFDEMO
					spritesound(S_THUNDER1, &sprite[plr.spritenum]);
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

void warp(int x, int y, int z, int daang, short dasector) {
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
	int endwall = (short) (startwall + sector[warpsect].wallnum - 1);
	int dax = 0, day = 0, i = 0;
	for (int s = startwall; s <= endwall; s++) {
		dax += wall[s].x;
		day += wall[s].y;
		if (wall[s].nextsector >= 0) {
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

void warpsprite(short spritenum) {
	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	if (monsterwarptime > 0)
		return;
	short dasectnum = sprite[spritenum].sectnum;
	warpfxsprite(spritenum);
	warp(sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z, sprite[spritenum].ang, dasectnum);
	sprite[spritenum].x = warpx;
	sprite[spritenum].y = warpy;
	sprite[spritenum].z = warpz;
	sprite[spritenum].ang = (short) warpang;
	dasectnum = (short) warpsect;

	warpfxsprite(spritenum);
	setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);

	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	monsterwarptime = 120;
}

void ironbars() {
	for (int i = 0; i < ironbarscnt; i++) {
		if (ironbarsdone[i] == 1) {
			short spritenum = ironbarsanim[i];
			switch (sprite[ironbarsanim[i]].hitag) {
			case 1:
				sprite[ironbarsanim[i]].ang += TICSPERFRAME << 1;
				if (sprite[ironbarsanim[i]].ang > 2047)
					sprite[ironbarsanim[i]].ang -= 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					sprite[ironbarsanim[i]].hitag = 2;
					ironbarsdone[i] = 0;
				}
				break;
			case 2:
				sprite[ironbarsanim[i]].ang -= TICSPERFRAME << 1;
				if (sprite[ironbarsanim[i]].ang < 0)
					sprite[ironbarsanim[i]].ang += 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					sprite[ironbarsanim[i]].hitag = 1;
					ironbarsdone[i] = 0;
				}
				break;
			case 3:
				sprite[ironbarsanim[i]].z -= TICSPERFRAME << 4;
				if (sprite[ironbarsanim[i]].z < ironbarsgoal[i]) {
					sprite[ironbarsanim[i]].z = ironbarsgoal[i];
					sprite[ironbarsanim[i]].hitag = 4;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = sprite[ironbarsanim[i]].z + 6000;
				}
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				break;
			case 4:
				sprite[ironbarsanim[i]].z += TICSPERFRAME << 4;
				if (sprite[ironbarsanim[i]].z > ironbarsgoal[i]) {
					sprite[ironbarsanim[i]].z = ironbarsgoal[i];
					sprite[ironbarsanim[i]].hitag = 3;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = sprite[ironbarsanim[i]].z - 6000;
				}
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				break;
			}
		}
	}
}

void sectorsounds() {
	if (!SoundEnabled())
		return;

	PLAYER& plr = player[pyrn];

	int sec = sector[plr.sector].extra & 0xFFFF;
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
			if (plr.z <= sector[plr.sector].floorz - (8 << 8))
				spritesound(sec, &sprite[plr.spritenum]);
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
		if (sector[plr.sector].lotag == 50 && sector[plr.sector].hitag > 0)
			weaponpowerup(plr);
	}

#pragma message ("drunk?")
#if 0
	GLRenderer gl = glrender();
	if (gl != nullptr) {
		if (player[pyrn].poisoned != 0) {
			int tilt = mulscale(sintable[(3 * lockclock) & 2047], 20, 16);
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

	if (sector[plr.sector].floorpicnum == WATER || sector[plr.sector].floorpicnum == LAVA
			|| sector[plr.sector].floorpicnum == SLIME) {
		if (plr.onsomething == 0)
			return;

		if (plr.sector != plr.oldsector) {
			switch (sector[plr.sector].floorpicnum) {
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
	int j = insertsprite(plr.sector, MASPLASH);
	sprite[j].x = plr.x;
	sprite[j].y = plr.y;
	sprite[j].z = sector[plr.sector].floorz + (tileHeight(picnum) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = (short) picnum;
	sprite[j].shade = 0;
	sprite[j].pal = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 8;
	sprite[j].hitag = 0;

	switch (picnum) {
	case SPLASHAROO:
	case SLIMESPLASH:
		if(!isWh2() && picnum == SLIMESPLASH)
			break;
			
		spritesound(S_SPLASH1 + (krand() % 3), &sprite[j]);
		break;
	case LAVASPLASH:
		break;
	}

	movesprite((short) j, ((sintable[(sprite[j].ang + 512) & 2047]) * TICSPERFRAME) << 3,
			((sintable[sprite[j].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
}

void makemonstersplash(int picnum, int i) {
	if (sprite[i].picnum == FISH)
		return;

	int j = insertsprite(sprite[i].sectnum, MASPLASH);
	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sector[sprite[i].sectnum].floorz + (tileHeight(picnum) << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = (short) picnum;
	sprite[j].shade = 0;

	if (sector[sprite[i].sectnum].floorpal == 9)
		sprite[j].pal = 9;
	else
		sprite[j].pal = 0;
		
	if (sprite[i].picnum == RAT) {
		sprite[j].xrepeat = 40;
		sprite[j].yrepeat = 40;
	} else {
		sprite[j].xrepeat = 64;
		sprite[j].yrepeat = 64;
	}
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 8;
	sprite[j].hitag = 0;

	// JSA 5/3 start
	switch (picnum) {
	case SPLASHAROO:
	case SLIME:
		if ((krand() % 2) != 0) {
			if ((gotpic[WATER >> 3] & (1 << (WATER & 7))) > 0) {
				gotpic[WATER >> 3] &= ~(1 << (WATER & 7));
					if ((krand() % 2) != 0)
						spritesound(S_SPLASH1 + (krand() % 3), &sprite[j]);
			}
		}
		if ((krand() % 2) != 0) {
			if ((gotpic[SLIME >> 3] & (1 << (SLIME & 7))) > 0) {
				gotpic[SLIME >> 3] &= ~(1 << (SLIME & 7));
					if ((krand() % 2) != 0)
						spritesound(S_SPLASH1 + (krand() % 3), &sprite[j]);
			}
		}
		break;
	case LAVASPLASH:
		break;
	}
}

void bats(PLAYER& plr, int k) {
	short j = insertsprite(sprite[k].sectnum, FLOCK);
	sprite[j].x = sprite[k].x;
	sprite[j].y = sprite[k].y;
	sprite[j].z = sprite[k].z;
	sprite[j].cstat = 0;
	sprite[j].picnum = BAT;
	sprite[j].shade = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short) ((sprite[k].ang + (krand() & 128 - 256)) & 2047);
	sprite[j].owner = (short) k;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 128;
	sprite[j].hitag = (short) k;
	sprite[j].extra = 0;

	newstatus(j, FLOCK);

	if (sprite[k].extra == 1)
		lastbat = j;
}

void cracks() {

	PLAYER& plr = player[0];
	if (plr.sector == -1)
		return;

	int datag = sector[plr.sector].lotag;

	if (floorpanningcnt < 64)
		if (datag >= 3500 && datag <= 3599) {
			sector[plr.sector].hitag = 0;
			int daz = sector[plr.sector].floorz + (1024 * (sector[plr.sector].lotag - 3500));
			if ((setanimation(plr.sector, daz, 32, 0, FLOORZ)) >= 0) {
				sector[plr.sector].floorpicnum = LAVA1;
				sector[plr.sector].floorshade = -25;
				SND_Sound(S_CRACKING);
			}
			sector[plr.sector].lotag = 80;
			floorpanninglist[floorpanningcnt++] = plr.sector;
		}

	if (datag >= 5100 && datag <= 5199) {
		sector[plr.sector].hitag = 0;
		sector[plr.sector].lotag = 0;
	}

	if (datag >= 5200 && datag <= 5299) {
		sector[plr.sector].hitag = 0;
		sector[plr.sector].lotag = 0;
	}

	if (datag == 3001) {
		sector[plr.sector].lotag = 0;
		for (short k = 0; k < MAXSPRITES; k++) {
			if (sector[plr.sector].hitag == sprite[k].hitag) {
				sprite[k].lotag = 36;
				sprite[k].zvel = (short) (krand() & 1024 + 512);
				newstatus(k, SHOVE);
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

void warpfxsprite(int s) {
	PLAYER& plr = player[pyrn];

	int j = insertsprite(sprite[s].sectnum, WARPFX);

	sprite[j].x = sprite[s].x;
	sprite[j].y = sprite[s].y;
	sprite[j].z = sprite[s].z - (32 << 8);

	sprite[j].cstat = 0;

	sprite[j].picnum = ANNIHILATE;
	short daang;
	if (s == plr.spritenum) {
		daang = plr.angle.ang.asbuild();
		sprite[j].ang = daang;
	} else {
		daang = sprite[s].ang;
		sprite[j].ang = daang;
	}

	sprite[j].xrepeat = 48;
	sprite[j].yrepeat = 48;
	sprite[j].clipdist = 16;

	sprite[j].extra = 0;
	sprite[j].shade = -31;
	sprite[j].xvel = (short) ((krand() & 256) - 128);
	sprite[j].yvel = (short) ((krand() & 256) - 128);
	sprite[j].zvel = (short) ((krand() & 256) - 128);
	sprite[j].owner = (short) s;
	sprite[j].lotag = 12;
	sprite[j].hitag = 0;
	sprite[j].pal = 0;

	int daz = (((sprite[j].zvel) * TICSPERFRAME) >> 3);

	movesprite((short) j, ((sintable[(daang + 512) & 2047]) * TICSPERFRAME) << 3,
			((sintable[daang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);
}

void FadeInit() {
#pragma message("Fix fades")
#if 0
	Console.Println("Initializing fade effects", 0);
	registerFade("RED", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				r = 3 * (intensive + 32);
				a = 2 * (intensive + 32);
			} else
				r = a = 0;
			if (r > 255)
				r = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(r, 0, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			int multiple = intensive / 2;
			if (multiple > 170)
				multiple = 170;
			gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gl.glColor4ub(r > 0 ? multiple : 0, 0, 0, 0);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();
		}
	});

	registerFade("WHITE", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				g = r = 10 * intensive;
				a = (intensive + 32);
			} else
				g = r = a = 0;

			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(r, g, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(r > 0 ? multiple : 0, g > 0 ? multiple : 0, 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});

	registerFade("GREEN", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				g = 4 * intensive;
				a = (intensive + 32);
			} else
				g = a = 0;

			if (g > 255)
				g = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(0, g, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(0, g > 0 ? multiple : 0, 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});

	registerFade("BLUE", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				b = 4 * intensive;
				a = (intensive + 32);
			} else
				b = a = 0;

			if (b > 255)
				b = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(0, 0, b, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(0, 0, b > 0 ? multiple : 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});
#endif
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
				
			if (sector[plr.sector].hitag > 0) {
				sector[plr.sector].hitag--;
				if (sector[plr.sector].hitag == 0) {
					short j = headspritesect[plr.sector];
					while (j != -1) {
						short nextj = nextspritesect[j];
						if (sprite[j].picnum == CONE) {
							deletesprite(j);
						} else if (sprite[j].picnum == SPARKBALL) {
							deletesprite(j);
						}
						j = nextj;
					}
				}
			}
		}
	}
}

void makesparks(short i, int type) {

	int j = -1;

	switch (type) {
	case 1:
		j = insertsprite(sprite[i].sectnum, SPARKS);
		break;
	case 2:
		j = insertsprite(sprite[i].sectnum, SPARKSUP);
		break;
	case 3:
		j = insertsprite(sprite[i].sectnum, SPARKSDN);
		break;
	}

	if (j == -1)
		return;

	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sprite[i].z;
	sprite[j].cstat = 0;
	sprite[j].picnum = SPARKBALL;
	sprite[j].shade = 0;
	sprite[j].xrepeat = 24;
	sprite[j].yrepeat = 24;
	sprite[j].ang = (short) ((krand() % 2047) & 2047);
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = (short) (krand() % 100);
	sprite[j].hitag = 0;
	sprite[j].extra = 0;

	sprite[j].pal = 0;
}

void shards(int i, int type) {
	short j = insertsprite(sprite[i].sectnum, SHARDOFGLASS);

	sprite[j].x = sprite[i].x + (((krand() % 512) - 256) << 2);
	sprite[j].y = sprite[i].y + (((krand() % 512) - 256) << 2);
	sprite[j].z = sprite[i].z - (getPlayerHeight() << 8) + (((krand() % 48) - 16) << 7);
	sprite[j].zvel = (short) (krand() % 256);
	sprite[j].cstat = 0;
	sprite[j].picnum = (short) (SHARD + (krand() % 3));
	sprite[j].shade = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short) (sprite[i].ang + ((krand() % 512) - 256) & 2047);
	sprite[j].owner = (short) i;
	sprite[j].clipdist = 16;
	sprite[j].lotag = (short) (120 + (krand() % 100));
	sprite[j].hitag = 0;
	sprite[j].extra = (short) type;
	sprite[j].pal = 0;
}


END_WH_NS
