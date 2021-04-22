// contains all global Duke definitions
struct Duke native
{
	enum ESpecialMusic
	{
		MUS_INTRO = 0,
		MUS_BRIEFING = 1,
		MUS_LOADING = 2,
	};
	
	enum EPalette
	{
		BASEPAL = 0,
		WATERPAL,
		SLIMEPAL,
		TITLEPAL,
		DREALMSPAL,
		ENDINGPAL,  // 5
		ANIMPAL,    // not used anymore. The anim code now generates true color textures.
		DRUGPAL,
		BASEPALCOUNT
	};
	

	native static void PlaySpecialMusic(int which);
}
