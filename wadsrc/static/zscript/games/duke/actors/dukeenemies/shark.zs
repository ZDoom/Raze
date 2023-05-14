
class DukeShark : DukeActor
{
	const SHARKSTRENGTH = 35;
	const SHARKBITESTRENGTH = -9;
	
	default
	{
		pic "SHARK";
		+INTERNAL_BADGUY;
		+DONTDIVEALIVE;
		+FLOATING;
		Strength SHARKSTRENGTH;
	}
	
	override void Initialize(DukeActor spawner)
	{
		// override some defaults.
		self.scale = (0.9375, 0.9375);
		self.clipdist = 10;
	}

}