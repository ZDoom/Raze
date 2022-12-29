class DukeBoss3 : DukeBoss1
{
	default
	{
		pic "BOSS3";
		-ALTHITSCANDIRECTION;
		-DONTENTERWATER;
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
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
	
}
	