class RedneckMinion : DukeActor
{
	const MINIONSTRENGTH = 50;
	const MINIONFREAK = -10;

	default
	{
		pic "MINION";
		strength MINIONSTRENGTH;
		precacheclass "RedneckMinJibA", "RedneckMinJibB", "RedneckMinJibC";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUNDWITHPAL8;
		+TRANSFERPALTOJIBS;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
		if (ud.ufospawnsminion)
			self.pal = 8;
		if (self.pal == 19)
		{
			self.bHitradius_NoEffect = true;
			self.bMagmaImmune = true;
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
		+BADGUYSTAYPUT;
	}

	override void PlayFTASound(int mode)
	{
	}

}

