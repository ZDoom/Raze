class DukeBoss2 : DukeBoss1
{
	default
	{
		pic "BOSS2";

		+NONSMOKYROCKET; // If this wasn't needed for a CON defined actor it could be handled better
	}
	
	override void PlayFTASound()
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
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
	
}
	
