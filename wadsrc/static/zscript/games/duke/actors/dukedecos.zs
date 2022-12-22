class DukeHorseOnSide : DukeActor
{
	default
	{
		pic "HORSEONSIDE";
		+FORCERUNCON;
		+CHECKSLEEP;
		+MOVEFTA_MAKESTANDABLE;
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


class DukeExplodingBarrel : DukeHorseOnSide
{
	default
	{
		pic "EXPLODINGBARREL";
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
	}
}

class DukeFireBarrel : DukeHorseOnSide
{
	default
	{
		pic "FIREBARREL";
	}
}
class DukeNukeBarrel : DukeHorseOnSide
{
	default
	{
		pic "NUKEBARREL";
		+GREENBLOOD;
	}
}
class DukeFireVase : DukeHorseOnSide
{
	default
	{
		pic "FIREVASE";
	}
}
class DukeNukeBarrelDented : DukeHorseOnSide
{
	default
	{
		pic "NUKEBARRELDENTED";
	}
}
class DukeNukeBarrelLeaked : DukeHorseOnSide
{
	default
	{
		pic "NUKEBARRELLEAKED";
	}
}
class DukeWoodenHorse : DukeHorseOnSide
{
	default
	{
		pic "WOODENHORSE";
	}
}

class DukeRubberCan : DukeHorseOnSide
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
		+FORCERUNCON;
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
		+FORCERUNCON;
		+BROWNBLOOD;
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
		pic "BLOOD";
		+FORCERUNCON;
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
		+FORCERUNCON;
	}
	
	override void Initialize()
	{
		self.pos.X += frandom(-8, 8);
		self.pos.Y += frandom(-8, 8);
		self.scale = (0.3125, 0.3125);
		self.ChangeStat(STAT_MISC);
	}
}		

class DukeLavaSplash : DukeActor
{
	default
	{
		pic "LAVASPLASH";
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

