

class RedneckCoot : DukeActor
{
	const COOTSTRENGTH = 50;

	default
	{
		pic "COOT";
		Strength COOTSTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+DESTRUCTOIMMUNE;
	}
	
	override void PlayFTASound()
	{
		if (!Raze.isRRRA() && (random(0, 3)) == 2)
			self.PlayActorSound("CT_GET");
	}
	
	override void Initialize()
	{
		self.scale = (0.375, 0.28125);
		self.setClipDistFromTile();
		self.clipdist *= 4;
		if (Raze.isRRRA()) bLookAllaround = true;
	}

	
}

class RedneckCootStayput: RedneckCoot
{
	default
	{
		pic "COOTSTAYPUT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+BADGUYSTAYPUT;
	}

	override void PlayFTASound()
	{
	}
	
}

