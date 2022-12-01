class DukeNaturalLightning : DukeActor
{
	default
	{
		pic "NATURALLIGHTNING";
	}
	
	override void Initialize()
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
	}
	
	override bool Animate(tspritetype t)
	{
		t.shade = -127;
		return true;
	}
}
