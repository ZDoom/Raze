// NWinter new stuff

class NWinterNoisemaker : DukeActor // NOISEMAKER (3883)
{
	default
	{
		pic "NOISEMAKER";
		Strength 999;
		action "VOIDACTION", 0;
		move "NULLSPEED";
		ai "MAKENOISE", "VOIDACTION", "NULLSPEED", geth| getv;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.actorsizeto(48 * REPEAT_SCALE, 40 * REPEAT_SCALE);
		self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_ALIGNMENT_WALL;
		if (self.curAI == 'none')
		{
			setAI('MAKENOISE');
		}
		if (self.pal == 21)
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.counter >= 24)
				{
					self.extra = 999;
					if (Duke.rnd(128))
					{
						self.PlayActorSound("MOVIE2");
					}
					else
					{
						self.PlayActorSound("MOVIE3");
					}
					self.counter = 0;
				}
			}
		}
		else
		{
			self.hitradius(4096, 1, 1, 1, 1);
		}
	}
	
}
class NWinterMaleGeek : DukeActor // MALEGEEK (3761)
{
	default
	{
		pic "MALEGEEK";
		Strength 20;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.ifhitbyweapon() >= 0 || self.extra < 0)
		{
			Duke.StopSound("MALECAROL");
			self.spawnguts('DukeJibs6', 2);
			self.spawnguts('DukeJibs6', 3);
			self.spawnguts('DukeJibs6', 4);
			self.PlayActorSound("MALECAROL_DEAD");
			self.PlayActorSound("MOUSEANNOY");
			self.killit();
		}
		if (self.counter >= 225)
		{
			if (self.cansee(p))
			{
				self.PlayActorSound("MALECAROL");
			}
			self.counter = 0;
		}
	}
	
}
class NWinterFemaleGeek : DukeActor // FEMALEGEEK (3823)
{
	default
	{
		pic "FEMALEGEEK";
		Strength 20;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.ifhitbyweapon() >= 0 || self.extra < 0)
		{
			Duke.StopSound("FEMALECAROL");
			self.spawnguts('DukeJibs6', 2);
			self.spawnguts('DukeJibs6', 3);
			self.spawnguts('DukeJibs6', 4);
			self.PlayActorSound("FEMALECAROL_DEAD");
			self.killit();
		}
		if (self.counter >= 225)
		{
			if (self.cansee(p))
			{
				self.PlayActorSound("FEMALECAROL");
			}
			self.counter = 0;
		}
	}
	
}
class NWinterSnowmaker : DukeActor // SNOWMAKER (3787)
{
	default
	{
		pic "SNOWMAKER";
		Strength 0;
		StartAction "none";
		StartMove "none";
		action "SNOWMAKE", 0;
		move "SNOWMAKERMOVE", 100, 0;
		ai "AISNOWMAKERROAM", "SNOWMAKE", "SNOWMAKERMOVE", getv| geth| randomangle;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		if (self.curAI == 'none')
		{
			if (self.sector != null) self.spawn('NWinterSnowflake');
			setAI('AISNOWMAKERROAM');
			return;
		}
		if (self.movflag > kHitSector)
		{
			setAI('AISNOWMAKERROAM');
		}
		if (self.pal == 0)
		{
			if (self.counter >= 48)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				setAI('AISNOWMAKERROAM');
				self.counter = 0;
			}
		}
		if (self.pal == 1)
		{
			if (self.counter >= 32)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				setAI('AISNOWMAKERROAM');
				self.counter = 0;
			}
		}
		if (self.pal == 2)
		{
			if (self.counter >= 64)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				setAI('AISNOWMAKERROAM');
				self.counter = 0;
			}
		}
		if (self.pal == 3)
		{
			if (self.counter >= 96)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				setAI('AISNOWMAKERROAM');
				self.counter = 0;
			}
		}
		if (self.pal == 4)
		{
			if (self.counter >= 16)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				setAI('AISNOWMAKERROAM');
				self.counter = 0;
			}
		}
	}
	
}

class NWinterSnowmakerNomove : DukeActor // SNOWMAKERNOMOVE (3789)
{
	default
	{
		pic "SNOWMAKERNOMOVE";
		Strength 0;
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		if (self.pal == 0)
		{
			if (self.counter >= 32)
			{
				if (self.sector != null) self.spawn('NWinterSnowflake');
				self.counter = 0;
			}
		}
		if (self.pal == 1)
		{
			if (self.counter >= 48)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				self.counter = 0;
			}
		}
		if (self.pal == 2)
		{
			if (self.counter >= 64)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				self.counter = 0;
			}
		}
		if (self.pal == 3)
		{
			if (self.counter >= 96)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				self.counter = 0;
			}
		}
		if (self.pal == 4)
		{
			if (self.counter >= 16)
			{
				if (self.cansee(p))
				{
					if (self.sector != null) self.spawn('NWinterSnowflake');
				}
				self.counter = 0;
			}
		}
	}
}


class NWinterSnowflake : DukeActor // SNOWFLAKE (3697)
{
	default
	{
		pic "SNOWFLAKE";
		Strength 0;
		action "SNOW", 0;
		move "SNOWMOVE", -25, 40;
		StartAction "SNOW";
		StartMove "SNOWMOVE";
		moveflags getv | geth | randomangle;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.actorsizeto(25 * REPEAT_SCALE, 25 * REPEAT_SCALE);
		self.cstat = 0;
		if (self.counter >= 4)
		{
			setMove('SNOWMOVE', getv | geth | randomangle);
			self.counter = 0;
			return;
		}
		if (self.floorz - self.pos.Z < 5)
		{
			self.killit();
		}
	}
	
}
