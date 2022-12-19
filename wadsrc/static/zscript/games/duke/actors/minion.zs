class RedneckMinion : DukeActor
{
	default
	{
		pic "MINION";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
		if (Raze.isRRRA() && ud.ufospawnsminion)
			self.pal = 8;
	}
	
	override bool animate(tspritetype t)
	{
		if (Raze.isRRRA() && t.pal == 19)
			t.shade = -127;
		return false;
	}
}

class RedneckMinionStayput: RedneckMinion
{
	default
	{
		pic "MINIONSTAYPUT";
	}

	override void PlayFTASound()
	{
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}
