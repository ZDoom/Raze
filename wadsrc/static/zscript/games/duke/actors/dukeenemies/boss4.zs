

class DukeBoss4 : DukeBossBase
{
	const BOSS4STRENGTH = 6000;
	const BOSS4PALSTRENGTH = 1000;

	default
	{
		pic "BOSS4";
		-ALTHITSCANDIRECTION;
		Strength BOSS4STRENGTH;

	}
	
	override void PlayFTASound(int mode)
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS4_RECOG");
		Duke.PlaySound("BOSS4_FIRSTSEE");
	}
}


class DukeBoss4Stayput : DukeBoss4
{
	default
	{
		pic "BOSS4STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	
}
	
