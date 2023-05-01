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
		action "EXPBARRELFRAME", 0, 2, 1, 1, 15;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curAction.name == 'EXPBARRELFRAME')
		{
			if (self.actioncounter >= 2)
			{
				self.hitradius(1024, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
				if (self.sector != null) self.spawn('DukeExplosion2');
				self.spawndebris(DukeScrap.Scrap2, 2);
				self.PlayActorSound("PIPEBOMB_EXPLODE");
				self.killit();
			}
			return;
		}
		if (self.ifsquished(p))
		{
			self.spawndebris(DukeScrap.Scrap1, 5);
			self.killit();
			return;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			setAction('EXPBARRELFRAME');
		}
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
		action "WOODENHORSEFRAME", 0, 1, 4;
		action "WOODENFALLFRAME", 122, 1, 5;
		StartAction "WOODENHORSEFRAME";
		Strength WEAKEST;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.spawndebris(DukeScrap.Scrap1, 4);
				self.spawndebris(DukeScrap.Scrap2, 3);
				self.killit();
			}
			else
			{
				setAction('WOODENFALLFRAME');
			}
		}
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
		StartAction "none";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeWoodenHorse');
		setAction('WOODENFALLFRAME');
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
		StartAction "none";
		StartMove "none";
	}
	
	override void Initialize()
	{
		self.hitag = 0;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (pdist < 3084 * maptoworld)
		{
			if (Duke.rnd(24))
			{
				if (self.sector != null) self.spawn('DukeWaterBubble');
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
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
		action "BUBBLE", 0;
		action "CRACKEDBUBBLE", 1;
		move "BUBMOVE", -10, -36;
		move "BUBMOVEFAST", -10, -52;
		Strength 0;
		StartAction "BUBBLE";
		StartMove "BUBMOVE";
		moveflags getv | geth | randomangle;

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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'CRACKEDBUBBLE')
		{
			if (self.sector.lotag == ST_2_UNDERWATER)
			{
				if (Duke.rnd(192))
				{
					self.killit();
				}
			}
			if (self.actioncounter >= 7)
			{
				self.killit();
			}
		}
		else
		{
			if (self.counter >= 4)
			{
				if (Duke.rnd(192))
				{
					setMove('BUBMOVE', getv | geth | randomangle);
				}
				else
				{
					setMove('BUBMOVEFAST', getv | geth | randomangle);
				}
				self.counter = 0;
				if (Duke.rnd(84))
				{
					self.Scale = (8 * REPEAT_SCALE, 10 * REPEAT_SCALE);
				}
				else if (Duke.rnd(84))
				{
					self.Scale = (10 * REPEAT_SCALE, 8 * REPEAT_SCALE);
				}
				else
				{
					self.Scale = (9 * REPEAT_SCALE, 9 * REPEAT_SCALE);
				}
			}
			if (abs(self.pos.Z - self.sector.floorz) < 32 && self.sector.lotag == ST_1_ABOVE_WATER)
			{
				if (self.floorz - self.pos.Z < 8)
				{
					setAction('CRACKEDBUBBLE');
				}
			}
			else
			{
				if (self.actioncounter >= 40)
				{
					setAction('CRACKEDBUBBLE');
				}
			}
		}
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
		action "BARREL_DENTING", 2, 2, 1, 1, 6;
		action "BARREL_DENTED", 1;
		action "BARREL_DENTED2", 2;
		move "SPAWNED_BLOOD";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.ifsquished(p))
		{
			self.spawndebris(DukeScrap.Scrap1, 32);
			if (self.sector != null) self.spawn('DukeBloodPool');
			state_random_ooz(p, pdist);
			self.killit();
		}
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curAction.name == 'BARREL_DENTING')
		{
			if (self.actioncounter >= 2)
			{
				self.spawndebris(DukeScrap.Scrap1, 10);
				if (Duke.rnd(2))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
				self.killit();
			}
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.extra < 0)
				{
					self.PlayActorSound("VENT_BUST");
					if (Duke.rnd(128))
					{
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					setAction('BARREL_DENTING');
				}
				else
				{
					if (self.curAction.name == 'none')
					{
						setAction('BARREL_DENTED');
					}
					else if (self.curAction.name == 'BARREL_DENTED')
					{
						setAction('BARREL_DENTED2');
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					else
					{
						if (self.curAction.name == 'BARREL_DENTED2')
						{
							setAction('BARREL_DENTING');
						}
					}
				}
			}
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeNukeBarrel');
		setAction('BARREL_DENTED');
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeNukeBarrel');
		setAction('BARREL_DENTED2');
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

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.counter >= 32)
		{
			self.counter = 0;
			if (self.checkp(p, palive))
			{
				if (pdist < 1480 * maptoworld)
				{
					if (self.checkp(p, phigher))
					{
						p.addphealth(-1, self.bBIGHEALTH);
						p.pals = color(16, 16, 0, 0);
						if (Duke.rnd(96))
						{
							self.PlayActorSound("PLAYER_LONGTERM_PAIN");
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			self.PlayActorSound("VENT_BUST");
			self.spawndebris(DukeScrap.Scrap1, 10);
			if (Duke.rnd(128))
			{
				if (self.sector != null) self.spawn('DukeBurning');
			}
			else
			{
				if (self.sector != null) self.spawn('DukeBurning2');
			}
			self.killit();
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.counter >= 24)
		{
			if (pdist < RETRIEVEDISTANCE * maptoworld)
			{
				if (Duke.rnd(SWEARFREQUENCY))
				{
					self.PlayActorSound("PLAYER_STEPONFECES", CHAN_AUTO, CHANF_SINGULAR);
				}
				self.PlayActorSound("STEPNIT");
				if (self.sector != null) self.spawn('DukeBloodPool');
				self.killit();
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		else
		{
			self.actorsizeto(32 * REPEAT_SCALE, 32 * REPEAT_SCALE);
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.checkp(p, pfacing))
		{
			if (pdist < 1280 * maptoworld)
			{
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					self.ChangeType('DukeStatueFlash');
					setMove('none', 0);
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
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
		
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.counter >= 32)
		{
			self.ChangeType('DukeStatue');
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pfacing))
		{
			if (pdist < 1280 * maptoworld)
			{
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					let snd = Raze.FindSoundByResID(self.yint);
					if (!self.CheckSoundPlaying(snd)) self.PlayActorSound(snd, CHAN_VOICE);
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('ANULLACTION');
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.PlayActorSound("SQUISHED");
				self.spawnguts('DukeJibs5', 8);
				self.spawnguts('DukeJibs6', 9);
				self.killit();
			}
			else
			{
				self.spawnguts('DukeJibs6', 1);
			}
		}
		if (Duke.rnd(1))
		{
			if (self.sector != null) self.spawn('DukeWaterDrip');
		}
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
		action "ASPEAKERBROKE", 1;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			if (self.ifhitbyweapon() >= 0)
			{
				Duke.StopSound("STORE_MUSIC");
				self.PlayActorSound("STORE_MUSIC_BROKE", CHAN_AUTO, CHANF_SINGULAR);
				setAction('ASPEAKERBROKE');
			}
			else
			{
				if (pdist < 10240 * maptoworld)
				{
					self.PlayActorSound("STORE_MUSIC", CHAN_AUTO, CHANF_SINGULAR);
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK_HITSCAN;
			}
		}
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

