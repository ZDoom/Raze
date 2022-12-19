class DukeNaturalLightning : DukeActor
{
	default
	{
		pic "NATURALLIGHTNING";
		+FULLBRIGHT;
	}
	
	override void Initialize()
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
	}
}
