
class DukeExplodingBarrel : DukeActor
{
	default
	{
		pic "EXPLODINGBARREL";
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned)
			self.scale = (0.5, 0.5);
		self.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
		self.clipdist = 18;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
}

class DukeHorseOnSide : DukeExplodingBarrel
{
	default
	{
		pic "HORSEONSIDE";
	}
}

class DukeFireBarrel : DukeExplodingBarrel
{
	default
	{
		pic "FIREBARREL";
	}
}
class DukeNukeBarrel : DukeExplodingBarrel
{
	default
	{
		pic "NUKEBARREL";
	}
}
class DukeFireVase : DukeExplodingBarrel
{
	default
	{
		pic "FIREVASE";
	}
}
class DukeNukeBarrelDented : DukeExplodingBarrel
{
	default
	{
		pic "NUKEBARRELDENTED";
	}
}
class DukeNukeBarrelLeaked : DukeExplodingBarrel
{
	default
	{
		pic "DTILE_NUKEBARRELLEAKED";
	}
}
class DukeWoodenHorse : DukeExplodingBarrel
{
	default
	{
		pic "WOODENHORSE";
	}
}

class DukeRubberCan : DukeExplodingBarrel
{
	default
	{
		pic "RUBBERCAN";
	}

	override void Initialize()
	{
		super.Initialize();
		self.extra = 0;
	}
}
