class NWinterSnowball : DukeActor
{
	default
	{
		pic "SNOWBALL";
		move "SNOWBALLSPEED1", 750, 100;
		move "SNOWBALLUP", 0, -2000;
		action "SNOWBALL1", 0, 1, 1, 1, 1;
		ai "AISNOWBALL1", "SNOWBALL1", "SNOWBALLSPEED1", geth| getv;
		ai "AISNOWBALLUP", "SNOWBALL1", "SNOWBALLUP", geth| getv;
		Strength 0;
		StartAction "SNOWBALL1";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			self.Scale = (1 * REPEAT_SCALE, 1 * REPEAT_SCALE);
			self.cstat = CSTAT_SPRITE_INVISIBLE;
			self.clipdist += 0 * 0.25;
			setAI('AISNOWBALLUP');
		}
		if (self.curAI == 'AISNOWBALLUP')
		{
			if (self.counter >= 1)
			{
				setAI('AISNOWBALL1');
				self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
				self.clipdist += 32 * 0.25;
				self.Scale = (30 * REPEAT_SCALE, 30 * REPEAT_SCALE);
			}
			else
			{
				return;
			}
		}
		if (self.curAI == 'AISNOWBALL1')
		{
			if (pdist < 1024 * maptoworld)
			{
				p.addphealth(-10, false);
				self.spawndebris(DukeScrap.Scrap3, 5);
				self.PlayActorSound("PLAYER_GRUNT");
				p.wackplayer();
				p.pals = color(15, 15, 15, 0);
				self.killit();
			}
			if (self.floorz - self.pos.Z < 10)
			{
				self.spawndebris(DukeScrap.Scrap3, 5);
				self.killit();
			}
		}
		if (self.counter >= 30)
		{
			self.killit();
		}
	}
	
}



class NWinterSnowman : DukeActor
{
	const SNOWMANSTRENGTH = 75;
	
	default
	{
		pic "SNOWMAN";
		+BADGUY
		+KILLCOUNT
		Strength SNOWMANSTRENGTH;
		action "ASNOWMANIDLE", 26, 3, 5, 1, 20;
		action "ASNOWMANATTACK", 11, 3, 5, 1, 30;
		action "ASNOWMANDIE", 5, 5, 1, 1, 10;
		action "ASNOWMANDEAD", 10, 1, 1;
		action "ASNOWMANFROZEN", 0, 1, 5, 1, 1;
		move "SNOWMANSPEED";
		ai "AISNOWFOLLOW", "ASNOWMANIDLE", "SNOWMANSPEED", faceplayerslow;
		ai "AISNOWATTACK", "ASNOWMANATTACK", "SNOWMANSPEED", faceplayer;
		ai "AISNOWMANSHRINK", "ASNOWMANIDLE", "SNOWMANSPEED", geth| getv;
		ai "AISNOWMANGROW", "ASNOWMANIDLE", "SNOWMANSPEED", geth| getv;
		ai "AISNOWMANDYING", "ASNOWMANDIE", "SNOWMANSPEED", geth| getv;
		StartAction "ASNOWMANIDLE";
	}
	
	void state_snowmanidlestate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (pdist < 8192 * maptoworld)
			{
				setAI('AISNOWATTACK');
				return;
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_snowmandyingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 5)
		{
			setAction('ASNOWMANDEAD');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_snowmanattackstate(DukePlayer p, double pdist)
	{
		if (self.counter >= 15)
		{
		}
		else
		{
			if (self.counter >= 14)
			{
				if (self.sector != null) self.spawn('NWinterSnowball');
			}
		}
		if (self.actioncounter >= 3)
		{
			self.counter = 0;
			if (pdist < 8192 * maptoworld)
			{
				self.actioncounter = 0;
			}
			else
			{
				setAI('AISNOWFOLLOW');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_snowmanshrinkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AISNOWFOLLOW');
		}
		else if (self.counter >= SHRUNKCOUNT)
		{
			self.actorsizeto(48 * REPEAT_SCALE, 40 * REPEAT_SCALE);
		}
		else
		{
			state_genericshrunkcode(p, pdist);
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_snowmanhitstate(DukePlayer p, double pdist)
	{
		if (self.extra < 0)
		{
			self.spawndebris(DukeScrap.Scrap3, 40);
			self.spawndebris(DukeScrap.Scrap3, 40);
			self.spawndebris(DukeScrap.Scrap3, 20);
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AISNOWMANGROW');
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('ASNOWMANFROZEN');
				self.extra = 0;
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.spawndebris(DukeScrap.Scrap3, 40);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.spawndebris(DukeScrap.Scrap3, 20);
				self.killit();
			}
			else
			{
				self.cstat = 0;
				setAI('AISNOWMANDYING');
			}
		}
		else
		{
			self.spawndebris(DukeScrap.Scrap3, 20);
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				self.PlayActorSound("ACTOR_SHRINKING");
				setAI('AISNOWMANSHRINK');
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.PlayActorSound("EXPANDERHIT");
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curAction.name == 'ASNOWMANDEAD')
		{
			if (ud.respawn_monsters) // for enemies:
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = SNOWMANSTRENGTH;
					setAI('AISNOWFOLLOW');
				}
			}
			else
			{
				self.extra = 0;
				if (self.ifhitbyweapon() >= 0)
				{
					if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
					{
						self.spawndebris(DukeScrap.Scrap3, 40);
						self.spawndebris(DukeScrap.Scrap3, 40);
						self.spawndebris(DukeScrap.Scrap3, 40);
						self.killit();
					}
				}
			}
			return;
		}
		else
		{
			if (self.curAction.name == 'ASNOWMANFROZEN')
			{
				if (self.counter >= THAWTIME)
				{
					setAI('AISNOWFOLLOW');
					self.pal = self.tempval;
					self.tempval = 0;
				}
				else
				{
					if (self.counter >= FROZENDRIPTIME)
					{
						if (Duke.rnd(8))
						{
							if (self.sector != null) self.spawn('DukeWaterDrip');
						}
					}
				}
				if (self.ifhitbyweapon() >= 0)
				{
					if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
					{
						self.extra = 0;
						return;
					}
					self.addkill();
					self.spriteglass(30);
					if (Duke.rnd(84))
					{
						self.spawndebris(DukeScrap.Scrap3, 10);
					}
					self.PlayActorSound("GLASS_BREAKING");
					self.killit();
				}
				if (self.checkp(p, pfacing))
				{
					if (pdist < FROZENQUICKKICKDIST * maptoworld)
					{
						p.playerkick(self);
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				return;
			}
		}
		if (self.curAI == 'AISNOWMANSHRINK')
		{
			state_snowmanshrinkstate(p, pdist);
			return;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_snowmanhitstate(p, pdist);
		}
		if (self.checkp(p, pdead))
		{
			return;
		}
		if (self.curAI == 'none')
		{
			setAI('AISNOWFOLLOW');
			self.Scale = (42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
			self.clipdist += 32 * 0.25;
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		}
		if (self.curAI == 'AISNOWMANDYING')
		{
			state_snowmandyingstate(p, pdist);
		}
		else if (self.curAI == 'AISNOWMANGROW')
		{
			if (self.counter >= 32)
			{
				self.addkill();
				self.PlayActorSound("SQUISHED");
				self.PlayActorSound("PIPEBOMB_EXPLODE");
				self.hitradius(2048, 60, 70, 80, 90);
				self.spawndebris(DukeScrap.Scrap3, 40);
				self.spawndebris(DukeScrap.Scrap3, 40);
				self.spawndebris(DukeScrap.Scrap3, 40);
				self.spawndebris(DukeScrap.Scrap3, 40);
				self.killit();
			}		
			else state_genericgrowcode(p, pdist);
		}
		else if (self.curAI == 'AISNOWFOLLOW')
		{
			state_snowmanidlestate(p, pdist);
		}
		else
		{
			if (self.curAI == 'AISNOWATTACK')
			{
				state_snowmanattackstate(p, pdist);
			}
		}
	}
	
}

class NWinterTank : DukeTank
{
	default
	{
		DukeTank.SpawnType "NWinterSnowman";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			self.Scale = (60 * REPEAT_SCALE, 60 * REPEAT_SCALE);
			setAction('ATANKWAIT');
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.clipdist += 100 * 0.25;
		}
		else Super.RunState(p, pdist);
	}
}
