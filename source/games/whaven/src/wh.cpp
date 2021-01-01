#include "ns.h"
#include "wh.h"

BEGIN_WH_NS 

// Placeholders that will go away.
int attacktheme;

int killcnt, kills;
int expgained;
int difficulty;
int lockclock;
int treasurescnt, treasuresfound;
SPRITE tspritelist[MAXSPRITESONSCREEN + 1];
int tspritelistcnt;
short arrowsprite[ARROWCOUNTLIMIT], throwpikesprite[THROWPIKELIMIT];
int sparksx, sparksy, sparksz;
int playertorch = 0;
uint8_t ceilingshadearray[MAXSECTORS];
uint8_t floorshadearray[MAXSECTORS];
uint8_t wallshadearray[MAXWALLS];
short floormirrorsector[64];
int floormirrorcnt;
int displaytime = -1;
int redcount, whitecount;

// We owe this to Java. Unfortunately WitchavenGDX was a bit sloppy with the use of its global variables so the wrapper is here to stay. :(
int zr_ceilz, zr_ceilhit, zr_florz, zr_florhit;
void getzrange(int x, int y, int z, short sectnum, int walldist, int cliptype) 
{
	::getzrange(x, y, z, sectnum, &zr_ceilz, &zr_ceilhit, &zr_florz, &zr_florhit, walldist, cliptype);
}

//This was done better. Strange.
void   neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, Neartag& nt, int32_t neartagrange, uint8_t tagsearch)
{
	::neartag(xs, ys, zs, sectnum, ange, &nt.tagsector, &nt.tagwall, &nt.tagsprite, &nt.taghitdist, neartagrange, tagsearch);
}

int hitscan(int xs, int ys, int zs, short sectnum, int vx, int vy, int vz, Hitscan& hit, int cliptype)
{
	return ::hitscan(xs, ys, zs, sectnum, vx, vy, vz, &hit.hitsect, &hit.hitwall, &hit.hitsprite, &hit.hitx, &hit.hity, &hit.hitz, cliptype);
}

Point rotatepoint(int xpivot, int ypivot, int x, int y, short daang) { // jfBuild
	Point rotatepoint;
	int dacos = bcos(daang);
	int dasin = bsin(daang);
	x -= xpivot;
	y -= ypivot;
	rotatepoint.x = dmulscale(x, dacos, -y, dasin, 14) + xpivot;
	rotatepoint.y = dmulscale(y, dacos, x, dasin, 14) + ypivot;

	return rotatepoint;
}
END_WH_NS