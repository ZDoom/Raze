class RedneckDestructo : DukeActor
{
	default
	{
		pic "DESTRUCTO";
		statnum STAT_DESTRUCT;
		+DESTRUCTOIMMUNE;
		Strength MEGASTRENGTH;
		
	}
	
	override void Initialize()
	{
		// do NOT run the default init code for this one.
	}
	
	override bool animate(tspritetype t)
	{
		t.cstat |= CSTAT_SPRITE_INVISIBLE;
		return true;
	}
	
}

class RedneckDestroyTags : DukeActor
{
	default
	{
		pic "DESTROYTAGS";
		+DESTRUCTOIMMUNE;
	}
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
		self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.clipdist = 0.25;
		self.ChangeStat(STAT_DESTRUCT);
	}
}
