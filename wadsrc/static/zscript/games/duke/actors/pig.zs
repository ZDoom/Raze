class RedneckPig : DukeActor
{
	default
	{
		pic "PIG";
		+INTERNAL_BADGUY;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);;
		self.setClipDistFromTile();
	}
}

class RedneckPigStayput: RedneckPig
{
	default
	{
		pic "PIGSTAYPUT";
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}

class RedneckPigEat : RedneckPig
{
	default
	{
		pic "PIGEAT";
	}
}

