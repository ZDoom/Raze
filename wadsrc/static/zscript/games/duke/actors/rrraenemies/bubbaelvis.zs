class RedneckBubbaelvis : RedneckBubbaStand
{
	default
	{
		pic "SBSWIPE";
		+BADGUYSTAYPUT;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+BADGUYSTAYPUT;
		Strength 100;
	}
	
	override void initialize()
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}

}
