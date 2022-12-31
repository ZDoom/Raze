class RedneckCow : DukeActor
{
	const COWSTRENGTH = 50;

	default
	{
		pic "COW";
		+BADGUY;
		Strength COWSTRENGTH;
	}
	
	override void Initialize()
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}

}