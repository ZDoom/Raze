
class DukeFemale1 : DukeActor
{
	default
	{
		pic "FEM1";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
}
		
class DukeFemale2 : DukeFemale1
{
	default
	{
		pic "FEM2";
	}
}
	
class DukeFemale3 : DukeFemale1
{
	default
	{
		pic "FEM3";
	}
}
	
class DukeFemale4 : DukeFemale1
{
	default
	{
		pic "FEM4";
	}
}
	
class DukeFemale5 : DukeFemale1
{
	default
	{
		pic "FEM5";
	}
}
	
class DukeFemale6 : DukeFemale1
{
	default
	{
		pic "FEM6";
	}
}
	
class DukeFemale7 : DukeFemale1
{
	default
	{
		pic "FEM7";
	}
}
	
class DukeFemale8 : DukeFemale1
{
	default
	{
		pic "FEM8";
	}
}
	
class DukeFemale9 : DukeFemale1
{
	default
	{
		pic "FEM9";
	}
}
	
class DukeFemale10 : DukeFemale1
{
	default
	{
		pic "FEM10";
	}
}
	
class DukeNaked : DukeFemale1
{
	default
	{
		pic "NAKED1";
	}
}
	
class DukeToughGal : DukeFemale1
{
	default
	{
		pic "TOUGHGAL";
	}
}
	
class DukeBloodyPole : DukeFemale1
{
	default
	{
		pic "BLOODYPOLE";
	}
}
	
class DukePodFemale : DukeFemale1
{
	default
	{
		pic "PODFEM1";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		self.extra <<= 1;
	}
	
}
	