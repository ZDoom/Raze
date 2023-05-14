class RedneckCow : DukeActor
{
	const COWSTRENGTH = 50;

	default
	{
		pic "COW";
		+BADGUY;
		+NODAMAGETURN;
		Strength COWSTRENGTH;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}

}