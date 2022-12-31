
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

class DukeLaserSite : DukeActor
{
	default
	{
		pic "LASERSITE";
		+FORCERUNCON;
	}
}

class RedneckUfoLight : DukeActor
{
	default
	{
		pic "UFOLIGHT";

	}
}

class DukeSpaceShuttle : DukeActor // (487)
{
	default
	{
		pic "SPACESHUTTLE";
	}
}

class DukeSatellite2 : DukeActor // (489)
{
	default
	{
		pic "SATELLITE";
	}
}

class DukeDevastatorBlast : DukeActor // (1642) looks dead.
{
	default
	{
		pic "DEVISTATORBLAST";
		//Strength FREEZETHROWER_WEAPON_STRENGTH;
	}
}

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
