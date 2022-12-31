class RedneckBubbaStand : DukeActor
{
	default
	{
		pic "BUBBASTAND";
		+INTERNAL_BADGUY;
		Strength 100;
	}
	
	override void initialize()
	{
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}

	
}
