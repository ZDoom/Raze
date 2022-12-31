class DukeBoss2 : DukeBossBase
{
	const BOSS2STRENGTH = 4500;
	const BOSS2PALSTRENGTH = 1000;

	default
	{
		pic "BOSS2";
		-ALTHITSCANDIRECTION;
		+NONSMOKYROCKET; // If this wasn't needed for a CON defined actor it could be handled better
		+SPECIALINIT;
		+ST3CONFINED;
		+DONTENTERWATER;
		Strength BOSS2STRENGTH;

	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void PlayFTASound(int mode)
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS2_RECOG");
		else Duke.PlaySound("WHIPYOURASS");
	}
	
}


class DukeBoss2Stayput : DukeBoss2
{
	default
	{
		pic "BOSS2STAYPUT";
		+BADGUYSTAYPUT;
	}
	
}
	
