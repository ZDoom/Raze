class RedneckMinion : DukeActor
{
	default
	{
		pic "MINION";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUNDWITHPAL8;
		+TRANSFERPALTOJIBS;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
		if (Raze.isRRRA())
		{
			if (ud.ufospawnsminion)
				self.pal = 8;

			if (self.pal == 19)
			{
				self.bHitradius_NoEffect = true;
				self.bMagmaImmune = true;
			}
		}
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
