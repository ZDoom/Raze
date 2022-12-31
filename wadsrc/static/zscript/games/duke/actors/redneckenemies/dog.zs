class RedneckDog : DukeActor
{
	default
	{
		pic "DOGRUN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		Strength 200;
	}
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
	}
	
	
}
