class RedneckCoot : DukeActor
{
	default
	{
		pic "COOT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
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
	}
}

class RedneckCootStayput: RedneckCoot
{
	default
	{
		pic "COOTSTAYPUT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
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

// CON for this is different in RRRA, so we need the split regardless of the flag.
class RedneckRACoot : RedneckCoot
{
	default
	{
		+LOOKALLAROUND
	}
}

class RedneckRACootStayput: RedneckRACoot
{
	default
	{
		pic "COOTSTAYPUT";
		+BADGUYSTAYPUT;
	}

	override void PlayFTASound()
	{
	}
}

