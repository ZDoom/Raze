class DukeSimpleItem : DukeActor
{
	default
	{
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeExplodingBarrel : DukeSimpleItem
{
	default
	{
		pic "EXPLODINGBARREL";
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
		Strength 26;
	}
	

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeWoodenHorse : DukeSimpleItem
{
	default
	{
		pic "WOODENHORSE";
		precacheClass "DukeHorseOnSide";
		Strength WEAKEST;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeHorseOnSide : DukeWoodenHorse
{
	default
	{
		pic "HORSEONSIDE";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeWaterbubbleMaker : DukeSimpleItem
{
	default
	{
		pic "WATERBUBBLEMAKER";
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize()
	{
		self.hitag = 0;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
		self.ChangeStat(STAT_STANDABLE);
	}
	

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeWaterBubble : DukeActor
{
	default
	{
		pic "WATERBUBBLE";
		+FORCERUNCON;
		+NOFLOORPAL;
		Strength 0;

	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner.isPlayer())
			self.pos.Z -= 16;
		if (owner != self) 
			self.angle = owner.angle;

		self.scale = (0.0625, 0.0625);
		self.ChangeStat(STAT_MISC);
	}

	override bool animate(tspritetype t)
	{
		if (dlevel.floorsurface(t.sector) == Duke.TSURF_SLIME)
		{
			t.pal = 7;
		}
		else
		{
			t.copyfloorpal(t.sector);
		}
		return false;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNukeBarrel : DukeSimpleItem
{
	default
	{
		pic "NUKEBARREL";
		+GREENBLOOD;
		Strength MEDIUMSTRENGTH;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNukeBarrelDented : DukeNukeBarrel
{
	default
	{
		pic "NUKEBARRELDENTED";
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNukeBarrelLeaked : DukeNukeBarrel
{
	default
	{
		pic "NUKEBARRELLEAKED";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFireBarrel : DukeSimpleItem
{
	default
	{
		pic "FIREBARREL";
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFireVase : DukeFireBarrel
{
	default
	{
		pic "FIREVASE";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeStatue : DukeActor
{
	default
	{
		pic "STATUE";
		+TRIGGERRESPAWN;
		+HITRADIUSCHECK;
	}
	
	override void Initialize()
	{
		self.clipdist = 32;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}

	override void OnHit(DukeActor proj)
	{
		self.lotsofcolourglass(40);
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.angle = frandom(0, 360);
		self.lotsofglass(8);
		self.Destroy();
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeStatueFlash : DukeStatue
{
	default
	{
		pic "STATUEFLASH";
	}
		
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeHotMeat : DukeActor // HOTMEAT (4427)
{
	default
	{
		pic "HOTMEAT";
		Strength TOUGH;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeSpeaker : DukeActor
{
	default
	{
		pic "SPEAKER";
		+NOFALLER;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

