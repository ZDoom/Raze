class RedneckBubbaStand : DukeActor
{
	default
	{
		pic "BUBBASTAND";
		+INTERNAL_BADGUY;
		+BADGUYSTAYPUT;
		Strength 100;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}

	
}
