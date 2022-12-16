
class DukePlug : DukeActor
{
	default
	{
		pic "PLUG";
	}
	
	override void Initialize()
	{
		self.lotag = 9999;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override bool OnUse(DukePlayer p)
	{
		p.actor.PlayActorSound("SHORT_CIRCUIT");
		p.actor.extra -= random(2, 5);
		p.pals = Color(32, 48, 48, 64);
		return true;
	}
}

class DukeFemMag : DukeActor
{
	default 
	{
		pic "FEMMAG1";
	}
	
	override void Initialize()
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		self.ChangeStat(STAT_DEFAULT);
	}
}

class DukeTag : DukeActor
{
	default 
	{
		pic "DUKETAG";
	}
	
	override void Initialize()
	{
		if (ud.multimode < 2 && self.pal)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else self.pal = 0;
	}
}

class DukeMaskWall : DukeActor
{
	default
	{
		pic "MASKWALL1";
	}
	
	override void Initialize()
	{
		let j = self.cstat & (CSTAT_SPRITE_ALIGNMENT_MASK | CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
		self.cstat = j | CSTAT_SPRITE_BLOCK;
		self.ChangeStat(STAT_DEFAULT);
	}
}

class DukeFootprints : DukeActor
{
	default
	{
		spriteset "FOOTPRINTS", "FOOTPRINTS2", "FOOTPRINTS3", "FOOTPRINTS4";
		+SE24_REMOVE;
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned)
		{
			bool away = self.isAwayFromWall(5.25);
			if (!away)
			{
				self.scale = (0, 0);
				return;
			}
			self.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
			self.insertspriteq();
		}
		let sect = self.sector;
		self.pos.Z = sect.floorz;
		if (sect.lotag != ST_1_ABOVE_WATER && sect.lotag != ST_2_UNDERWATER)
			self.scale = (0.5, 0.5);
		self.setSpriteSetImage(random(0, 3));
		self.ChangeStat(STAT_MISC);
	}
	
	override bool animate(tspritetype t)
	{
		if (self.shade == 127) t.shade = 127;
		if (t.pal == 6) t.shade = -127;
		return true;
	}
}

class DukeBulletHole : DukeActor
{
	default
	{
		pic "BULLETHOLE";
		+SE24_REMOVE;
		+NOTELEPORT;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | randomFlip();
		self.insertspriteq();
		self.Scale = (0.046875, 0.046875);
		self.ChangeStat(STAT_MISC);
	}
	
	override bool animate(tspritetype t)
	{
		t.shade = 16;
		return true;
	}	
}

class DukeGenericPole : DukeActor
{
	default
	{
		pic "GENERICPOLE";
	}
	
	override void Initialize()
	{
		if (ud.multimode < 2 && self.pal != 0)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else self.pal = 0;
	}
}

class DukeCameraPole : DukeGenericPole
{
	default
	{
		pic "CAMERAPOLE";
	}
	
	override void Initialize()
	{
		if (gs.camerashitable) self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		else self.cstat = 0;
		self.extra = 1;
		super.Initialize();
	}
}

class DukeNeon : DukeActor
{
	default
	{
		pic "NEON1";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.ChangeStat(STAT_MISC);
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

class DeveloperCommentary : DukeActor
{
	default
	{
		spriteset "DEVELOPERCOMMENTARY", "DEVELOPERCOMMENTARYON";
	}
	
	override bool TriggerSwitch(DukePlayer p)
	{
		if (!wt_commentary) return true;
		if (self.spriteSetIndex == 0)
		{
			if (Duke.StartCommentary(self.lotag, self))
				self.setSpriteSetImage(1);
		}
		else
		{
			Duke.StopCommentary();
			self.setSpriteSetImage(0);
		}
		return true;
	}

	override bool animate(tspritetype t)
	{
		if (!wt_commentary) t.scale = (0, 0);
		else t.shade = self.shade;
		return true;
	}
}

// These ones are either inert or use CON but also have flags attached

class DukeBarBroke : DukeActor
{
	default
	{
		pic "BARBROKE";
		+SE24_NOCARRY;
	}
}

class DukeBearingPlate : DukeActor
{
	default
	{
		pic "BEARINGPLATE";
	}
}

class DukeBurnedCorpse : DukeActor
{
	default
	{
		pic "BURNEDCORPSE";
		+FORCERUNCON;
	}
}

class DukeLaserSite : DukeActor
{
	default
	{
		pic "LASERSITE";
		+FORCERUNCON;
	}
}

class DukeSpeaker : DukeActor
{
	default
	{
		pic "SPEAKER";
		+NOFALLER;
	}
}

class RedneckUfoLight : DukeActor
{
	default
	{
		pic "UFOLIGHT";
	}
}

class DukeNewBeast : DukeActor
{
	default
	{
		pic "NEWBEAST";
		+BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+GREENBLOOD;
	}
}

class DukeNewBeastStayput : DukeNewBeast
{
	default
	{
		pic "NEWBEASTSTAYPUT";
		+BADGUYSTAYPUT;
	}
}

class DukeNewBeastHang : DukeNewBeast
{
	default
	{
		pic "NEWBEASTHANG";
	}
}

 class DukeTank : DukeActor
 {
 	default
	{
		pic "TANK";
		+BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+NORADIUSPUSH;
	}
}

class DukeFoodObject6: DukeActor
{
	default
	{
		pic "FOODOBJECT6";
	}
}

class DukeFoodObject11: DukeActor
{
	default
	{
		pic "FOODOBJECT11";
	}
}

class DukeFoodObject12: DukeActor
{
	default
	{
		pic "FOODOBJECT12";
	}
}
class DukeFoodObject13: DukeActor
{
	default
	{
		pic "FOODOBJECT13";
	}
}
class DukeFoodObject14: DukeActor
{
	default
	{
		pic "FOODOBJECT14";
	}
}
class DukeFoodObject15: DukeActor
{
	default
	{
		pic "FOODOBJECT15";
	}
}
class DukeFoodObject16: DukeActor
{
	default
	{
		pic "FOODOBJECT16";
	}
}
class DukeFoodObject17: DukeActor
{
	default
	{
		pic "FOODOBJECT17";
	}
}

class DukeSkinnedChicken: DukeActor
{
	default
	{
		pic "SKINNEDCHICKEN";
	}
}
class DukeShoppingCart: DukeActor
{
	default
	{
		pic "SHOPPINGCART";
	}
}
class DukeRobotDog2: DukeActor
{
	default
	{
		pic "ROBOTDOG2";
	}
}
class DukeFeatheredChicken: DukeActor
{
	default
	{
		pic "FEATHEREDCHICKEN";
	}
}
class DukeDolphin1: DukeActor
{
	default
	{
		pic "DOLPHIN1";
	}
}

class DukeDolphin2: DukeActor
{
	default
	{
		pic "DOLPHIN2";
	}
}



class DukeSnakep: DukeActor
{
	default
	{
		pic "SNAKEP";
	}
}

class DukeDonuts: DukeActor
{
	default
	{
		pic "DONUTS";
	}
}

class DukeDonuts2: DukeActor
{
	default
	{
		pic "DONUTS2";
	}
}

class DukeMailbag: DukeActor
{
	default
	{
		pic "MAILBAG";
	}
}

class DukeTeddybear: DukeActor
{
	default
	{
		pic "TEDDYBEAR";
	}
}

class DukeClock: DukeActor
{
	default
	{
		pic "CLOCK";
	}
}

class DukeBrokenClock: DukeActor
{
	default
	{
		pic "BROKENCLOCK";
	}
}

class DukeJollyMeal: DukeActor
{
	default
	{
		pic "JOLLYMEAL";
	}
}

class DukeGumballMachine: DukeActor
{
	default
	{
		pic "GUMBALLMACHINE";
	}
}

class DukeGumballMachineBroke: DukeActor
{
	default
	{
		pic "GUMBALLMACHINEBROKE";
	}
}

class DukeBurger: DukeActor
{
	default
	{
		pic "DUKEBURGER";
	}
}

class DukePoliceLightPole: DukeActor
{
	default
	{
		pic "POLICELIGHTPOLE";
	}
}

class DukeTopSecret: DukeActor
{
	default
	{
		pic "TOPSECRET";
	}
}

class DukeGunpowderbarrel: DukeActor
{
	default
	{
		pic "GUNPOWDERBARREL";
	}
}

class DukeFloorbasket: DukeActor
{
	default
	{
		pic "FLOORBASKET";
	}
}

class DukeRobotmouse: DukeActor
{
	default
	{
		pic "ROBOTMOUSE";
	}
}

class DukeRobotpirate: DukeActor
{
	default
	{
		pic "ROBOTPIRATE";
	}
}

class DukePirate1A: DukeActor
{
	default
	{
		pic "PIRATE1A";
	}
}

class DukeMan: DukeActor
{
	default
	{
		pic "MAN";
	}
}

class DukeMan2: DukeActor
{
	default
	{
		pic "MAN2";
	}
}

class DukePirate2A: DukeActor
{
	default
	{
		pic "PIRATE2A";
	}
}

class DukePirate3A: DukeActor
{
	default
	{
		pic "PIRATE3A";
	}
}

class DukePirate4A: DukeActor
{
	default
	{
		pic "PIRATE4A";
	}
}

class DukePirate5A: DukeActor
{
	default
	{
		pic "PIRATE5A";
	}
}

class DukePirate6A: DukeActor
{
	default
	{
		pic "PIRATE6A";
	}
}

class DukeFoodObject2: DukeActor
{
	default
	{
		pic "FOODOBJECT2";
	}
}

class DukeFem6Pad: DukeActor
{
	default
	{
		pic "FEM6PAD";
	}
}

class DukeCannonball: DukeActor
{
	default
	{
		pic "CANNONBALL";
	}
}

