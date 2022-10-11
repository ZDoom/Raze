
class CoreActor native
{
	native readonly sectortype sectp;
	
	native int16 cstat;
	//native int16 picnum; // access is disabled to allow later refactoring.
	native readonly Vector3 pos;
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

	native readonly int16 spritesetpic;
	native readonly int spawnindex;

	native void setpos(Vector3 newpos, bool relink = true);
	native void copypos(CoreActor newpos, bool relink = true);
	native void move(Vector3 newpos, bool relink = true);
	native void setSpritePic(int index);	// index into actor's spriteset.
	native void backuppos();
}

