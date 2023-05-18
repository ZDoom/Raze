class RedneckBubbaelvis : RedneckBubbaStand
{
	default
	{
		pic "BUBBAELVIS";
		+BADGUYSTAYPUT;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+BADGUYSTAYPUT;
		Strength 100;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}

}
