class DukeBoss4 : DukeBoss1
{
	default
	{
		pic "BOSS4";
	}
	
	override void PlayFTASound()
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
	}
}
	


