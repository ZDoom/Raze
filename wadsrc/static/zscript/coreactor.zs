
class CoreActor native
{
	//native readonly sectortype sectp;

	native int16 cstat;
	//native int16 picnum; // access is disabled to allow later refactoring.
	native readonly Vector3 pos;
	native readonly int16 statnum;
	//native int16 ang;
	native int16 xint;
	native int16 yint;
	native int16 inittype; // inittype, type and flags are for Blood.
	native int16 lotag, type;
	native int16 hitag, flags;
	native int16 extra;
	native int16 detail;

	native int8 shade;
	native uint8 pal;
	native uint8 clipdist;
	native uint8 blend;
	native uint8 xrepeat;
	native uint8 yrepeat;
	native int8 xoffset;
	native int8 yoffset;
	native int16 owner;
	native uint16 cstat2;

	native uint mdanimtims;
	native int16 mdanimcur;
	native int16 angoff, pitch, roll;
	//native vec3 pivot_offset, position_offset; // no access needed - these are display only.
	native uint8 renderflags;
	native float alpha;

	native readonly sectortype sector;
	native readonly int16 spritesetpic;
	native readonly int spawnindex;

	native void setpos(Vector3 newpos, bool relink = true);
	native void copypos(CoreActor newpos, bool relink = true);
	native void move(Vector3 newpos, bool relink = true);
	native void setSpritePic(int index);	// index into actor's spriteset.
	native void backuppos();
}

