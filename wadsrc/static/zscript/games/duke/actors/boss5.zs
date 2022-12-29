class DukeBoss5 : DukeBoss1
{
	default
	{
		pic "BOSS5";
		-ALTHITSCANDIRECTION;
		-DONTENTERWATER;
	}
}


class DukeBoss5Stayput : DukeBoss5
{
	default
	{
		pic "BOSS5STAYPUT";
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
	
}
	


