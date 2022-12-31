class DukeBoss3 : DukeBossBase
{
	const BOSS3STRENGTH = 4500;
	const BOSS3PALSTRENGTH = 1000;
	
	default
	{
		pic "BOSS3";
		-ALTHITSCANDIRECTION;
		Strength BOSS3STRENGTH;
		
	}
		
	override void PlayFTASound()
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS3_RECOG");
		else Duke.PlaySound("RIPHEADNECK");
	}


}


class DukeBoss3Stayput : DukeBoss3
{
	default
	{
		pic "BOSS3STAYPUT";
		+BADGUYSTAYPUT;
	}
	
}
	
