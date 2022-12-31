class RedneckPig : DukeActor
{
	const PIGSTRENGTH = 75;
	const PIG_GNAW_AMOUNT = -1;

	default
	{
		pic "PIG";
		+INTERNAL_BADGUY;
		Strength PIGSTRENGTH;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);;
		self.setClipDistFromTile();
	}
	
	
}

class RedneckPigStayput : RedneckPig
{
	default
	{
		pic "PIGSTAYPUT";
		+BADGUYSTAYPUT;
	}
	
}

class RedneckPigEat : RedneckPig
{
	default
	{
		pic "PIGEAT";
	}
	
	
}

