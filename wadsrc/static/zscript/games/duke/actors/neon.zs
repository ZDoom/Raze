class DukeNeon1 : DukeActor
{
	default
	{
		statnum STAT_MISC;
		pic "NEON1";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}

	override void Tick()
	{
		if ((Duke.global_random() / (self.lotag + 1) & 31) > 4) self.shade = -127;
		else self.shade = 127;
	}
	
	override bool Animate(tspritetype t)
	{
		t.shade = self.shade;
		return true;
	}
}

class DukeNeon2 : DukeNeon1
{
	default
	{
		pic "NEON2";
	}
}

class DukeNeon3 : DukeNeon1
{
	default
	{
		pic "NEON3";
	}
}

class DukeNeon4 : DukeNeon1
{
	default
	{
		pic "NEON4";
	}
}

class DukeNeon5 : DukeNeon1
{
	default
	{
		pic "NEON5";
	}
}

class DukeNeon6 : DukeNeon1
{
	default
	{
		pic "NEON6";
	}
}

