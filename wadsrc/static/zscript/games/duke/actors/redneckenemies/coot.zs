

class RedneckCoot : DukeActor
{
	const COOTSTRENGTH = 50;

	default
	{
		pic "COOT";
		Strength COOTSTRENGTH;
		precacheclass "RedneckCootJibA", "RedneckCootJibB", "RedneckCootJibC";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+DESTRUCTOIMMUNE;
	}
	
	override void PlayFTASound(int mode)
	{
		if (!isRRRA() && (random(0, 3)) == 2)
			self.PlayActorSound("CT_GET");
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.375, 0.28125);
		self.setClipDistFromTile();
		self.clipdist *= 4;
		if (isRRRA()) bLookAllaround = true;
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

	override void PlayFTASound(int mode)
	{
	}
	
}

