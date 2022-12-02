
enum EClipMask
{
	CLIPMASK0 = (1 << 16) + 1,
	CLIPMASK1 = (256 << 16) + 64
};

enum EHitBits
{
	kHitNone = 0,
	kHitSector = 0x4000,
	kHitWall = 0x8000,
	kHitSprite = 0xC000,
	kHitVoid = 0x10000,      // SW only
};

const MAXPLAYERS = 8;
const MAXSTATUS = 1024;
const zmaptoworld = (1. / 256.);
const maptoworld = (1. / 16.);

class CoreActor native
{
	const REPEAT_SCALE = 1. / 64.;
	native sectortype sector;	// cannot be read-only, some code calls clipmove directly on this.
	
	native int16 cstat;
	//native int16 picnum; // access is disabled to allow later refactoring.
	native Vector3 pos;
	native Vector3 opos;
	native readonly int16 statnum;
	native int16 intangle;
	native int16 xint;
	native int16 yint;
	native int16 inittype; // inittype, type and flags are for Blood.
	native int16 lotag, type;
	native int16 hitag, flags;
	native int16 extra;
	native int16 detail;

	native int8 shade;
	native uint8 pal;
	native uint8 intclipdist;
	native uint8 blend;
	native Vector2 scale;
	native int8 xoffset;
	native int8 yoffset;
	native int16 intowner;
	native uint16 cstat2;

	native uint mdanimtims;
	native int16 mdanimcur;
	native uint8 renderflags;
	native float alpha;
	native double clipdist;
	native double angle;
	native double pitch;
	native Vector3 vel;
	native double viewzoffset;

	native readonly int16 spritesetindex;
	native readonly int spawnindex;

	native void setpos(Vector3 newpos, bool relink = true);
	native void copypos(CoreActor newpos, bool relink = true);
	native void move(Vector3 newpos, bool relink = true);
	native void backuppos();
	native void setPosition(Vector3 pos);
	native void setPositionZ(Vector3 pos);
	native bool isAwayFromWall(double dist);

	native void ChangeSector(sectortype s, bool forcetail = false);
	native void ChangeStat(int s, bool forcetail = false);

	native clearscope static double deltaangle(double ang1, double ang2);
	native clearscope static double absangle(double ang1, double ang2);
	native clearscope static double Normalize180(double ang);

	int randomFlip()
	{
		int r = random(0, 3);
		if (r == 0) return 0;
		if (r == 1) return CSTAT_SPRITE_XFLIP;
		if (r == 2) return CSTAT_SPRITE_YFLIP;
		return CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP;
	}

	int randomXFlip()
	{
		int r = random(0, 1);
		if (r == 0) return 0;
		return CSTAT_SPRITE_XFLIP;
	}


}

