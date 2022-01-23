
class CoreActor native
{
	//native readonly sectortype sectp;

	native int16 cstat;
	native int16 picnum;
	native readonly int16 statnum;
	native int16 ang;
	native int16 xvel;
	native int16 yvel;
	native int16 zvel, inittype; // inittype, type and flags are for Blood.
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
	//native vec3 pivot_offset, position_offset;
	native uint8 renderflags;
	native float alpha;

	native readonly int spawnindex;
}

