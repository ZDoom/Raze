class RedneckCow : DukeActor
{
	default
	{
		pic "COW";
		+BADGUY;
	}
	
	override void Initialize()
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}
}

