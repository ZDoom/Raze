
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
		pic "NUKEBARRELLEAKED";
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

class DukeWaterdripSplash : DukeActor
{
	default
	{
		pic "WATERDRIPSPLASH";
	}
	
	override void Initialize()
	{
		self.scale = (0.375, 0.375);
		self.ChangeStat(STAT_STANDABLE);
	}	
}

class DukeWaterbubbleMaker : DukeActor
{
	default
	{
		pic "WATERBUBBLEMAKER";
	}
	
	override void Initialize()
	{
		self.hitag = 0;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
		self.ChangeStat(STAT_STANDABLE);
	}
}

class DukeFeces : DukeActor
{
	default
	{
		pic "FECES";
	}
	
	override void Initialize()
	{
		if (!mapSpawned)
			self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.ChangeStat(STAT_MISC);
	}
}

class DukeBlood : DukeActor
{
	default
	{
		pic "Blood";
	}
	
	override void Initialize()
	{
		self.pos.Z -= 26;
		if (!mapSpawned && self.ownerActor && self.ownerActor.pal == 6)
			self.pal = 6;
		self.scale = (0.25, 0.25);
		self.ChangeStat(STAT_MISC);
	}
}

class RedneckBlood : DukeBlood
{
	override void Initialize()
	{
		Super.Initialize();
		self.scale = (0.0625, 0.0625);
		self.pos.Z -= 26;
	}
}
		

class DukeBlimp : DukeActor
{
	default
	{
		pic "BLIMP";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}
}		

class DukeMike : DukeActor
{
	default
	{
		pic "MIKE";
	}
	
	override void Initialize()
	{
		self.yint = self.hitag;
		self.ChangeStat(STAT_ACTOR);
	}
}		

class DukeWhispySmoke : DukeActor
{
	default
	{
		pic "WHISPYSMOKE";
	}
	
	override void Initialize()
	{
		self.pos.X += frandom(-8, 8);
		self.pos.Y += frandom(-8, 8);
		self.scale = (0.3125, 0.3125);
		self.ChangeStat(STAT_MISC);
	}
}		

class DukeSeriousSam : DukeActor
{
	default
	{
		pic "SERIOUSSAM";
		statnum STAT_ZOMBIEACTOR;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.extra = 150;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
}		

