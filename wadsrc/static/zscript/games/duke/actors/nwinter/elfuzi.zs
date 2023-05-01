class NWinterElfUzi : DukeActor
{
	const ELFUZISTRENGTH = 50;
	const ELFUZIALTSTR = 25;
	
	default
	{
		pic "ELFUZI";
		+BADGUY
		+KILLCOUNT
		Strength ELFUZISTRENGTH;

		action "AELFUZIWALK", 0, 4, 5, 1, 30;
		action "AELFUZIRUN", 0, 4, 5, 1, 15;
		action "AELFUZIFROZEN", 0, 1, 5, 1;
		action "AELFUZIGROW", 0, 1, 5, 1;
		action "AELFUZIJUMP", 266, 2, 5, 1, 15;
		action "AELFUZIFALL", 276, 1, 5, 1, 1;
		action "AELFUZIDYING", 260, 5, 1, 1, 25;
		action "AELFUZIDEAD", 265, 1, 1, 1, 1;
		action "AELFUZISHOOT", 281, 2, 5, 1;
		move "elfuziwalkspeed", 100;
		move "elfuzirunspeed", 200;
		move "elfuzijumpspeed", 150, -200;
		move "elfuzifallspeed", 150, 150;
		move "elfuzistopped";
		move "elfuzishrink1", 80;
		ai "AIELFUZISHRINKING", "AELFUZIWALK", "elfuzishrink1", fleeenemy;
		ai "AIELFUZIDYING", "AELFUZIDYING", "elfuzistopped", faceplayer;
		ai "AIELFUZISEEKING", "AELFUZIWALK", "elfuziwalkspeed", seekplayer;
		ai "AIELFUZIHUNTING", "AELFUZIRUN", "elfuzirunspeed", faceplayer;
		ai "AIELFUZIJUMPING", "AELFUZIJUMP", "elfuzijumpspeed", geth| getv;
		ai "AIELFUZIFALLING", "AELFUZIFALL", "elfuzifallspeed", geth| getv;
		ai "AIELFUZISHOOTING", "AELFUZISHOOT", "elfuzistopped", faceplayer;
		ai "AIELFUZIIQ", "AELFUZIRUN", "elfuzirunspeed", geth| getv;
		ai "AIELFUZIGROW", "AELFUZIGROW", "elfuzistopped", geth| getv;
	}
	
	void state_elfuziseekstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.checkp(p, pdead))
			{
				return;
			}
			if (pdist < 15000 * maptoworld)
			{
				if (Duke.rnd(16))
				{
					setAI('AIELFUZIHUNTING');
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

	void state_elfuzihuntstate(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pdead))
		{
			setAI('AIELFUZISEEKING');
		}
		if (self.cansee(p))
		{
			if (pdist < 2000 * maptoworld)
			{
				setAI('AIELFUZISHOOTING');
				return;
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			if (Duke.rnd(64))
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIELFUZIJUMPING');
					return;
				}
			}
			if (Duke.rnd(7))
			{
				if (pdist < 8192 * maptoworld)
				{
					setAI('AIELFUZISHOOTING');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		else
		{
			if (Duke.rnd(64))
			{
				setAI('AIELFUZIIQ');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuziiqstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			setAI('AIELFUZIHUNTING');
		}
		else
		{
			if (Duke.rnd(16))
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIELFUZISEEKING');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuzishootstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.checkp(p, pdead))
			{
				setAI('AIELFUZISEEKING');
			}
			if (self.actioncounter >= 3)
			{
				if (self.pal == 21)
				{
					if (Duke.rnd(80))
					{
						self.PlayActorSound("SHRINKER_FIRE");
						self.shoot('DukeShrinker');
					}
				}
				else
				{
					self.PlayActorSound("CHAINGUN_FIRE");
					self.shoot('DukeShotSpark');
				}
				self.actioncounter = 0;
			}
			if (Duke.rnd(4))
			{
				setAI('AIELFUZIHUNTING');
			}
		}
		else
		{
			setAI('AIELFUZIIQ');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuzijumpingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 2)
		{
			setAI('AIELFUZIFALLING');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuzifallingstate(DukePlayer p, double pdist)
	{
		if (self.floorz - self.pos.Z < 5)
		{
			setAI('AIELFUZIHUNTING');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuzihitstate(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('NWinterSpecBlood');
		state_random_wall_jibs(p, pdist);
		if (self.extra < 0)
		{
			state_random_wall_jibs(p, pdist);
			if (Duke.rnd(128))
			{
				if (self.sector != null) self.spawn('NWinterXmasPresent2');
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AIELFUZIGROW');
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('AELFUZIFROZEN');
				self.extra = 0;
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else
			{
				setAI('AIELFUZIDYING');
			}
		}
		else
		{
			state_random_wall_jibs(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				self.PlayActorSound("ACTOR_SHRINKING");
				setAI('AIELFUZISHRINKING');
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			else
			{
				if (Duke.rnd(32))
				{
					setAI('AIELFUZISHOOTING');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfuzishrinkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AIELFUZISEEKING');
		}
		else if (self.counter >= SHRUNKCOUNT)
		{
			self.actorsizeto(36 * REPEAT_SCALE, 30 * REPEAT_SCALE);
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

	void state_elfuzidyingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 5)
		{
			if (Duke.rnd(64))
			{
				if (self.sector != null) self.spawn('DukeBloodPool');
			}
			state_rf(p, pdist);
			if (self.floorz - self.pos.Z < 8)
			{
				self.PlayActorSound("THUD");
			}
			setAction('AELFUZIDEAD');
			setMove('elfuzistopped', 0);
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
		state_checksquished(p, pdist);
		if (self.curAction.name == 'AELFUZIDEAD')
		{
			if (ud.respawn_monsters) // for enemies:
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = ELFUZISTRENGTH;
					setAI('AIELFUZISEEKING');
				}
			}
			else
			{
				self.extra = 0;
				if (self.ifhitbyweapon() >= 0)
				{
					if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
					{
						self.PlayActorSound("SQUISHED");
						state_standard_jibs(p, pdist);
						state_standard_jibs(p, pdist);
						state_standard_jibs(p, pdist);
						self.killit();
					}
				}
			}
			return;
		}
		else if (self.curAction.name == 'AELFUZIFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIELFUZISEEKING');
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
					if (self.sector != null) self.spawn('DukeBloodPool');
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
		else
		{
			if (self.curAI == 'AIELFUZIDYING')
			{
				state_elfuzidyingstate(p, pdist);
			}
		}
		if (self.curAI == 'AIELFUZISHRINKING')
		{
		}
		else
		{
			if (self.curAI == 'AIELFUZIGROW')
			{
			}
			else
			{
				if (self.ifhitbyweapon() >= 0)
				{
					state_elfuzihitstate(p, pdist);
				}
			}
		}
		if (self.curAI == 'none')
		{
			setAI('AIELFUZISEEKING');
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.clipdist += 32 * 0.25;
			self.Scale = (32 * REPEAT_SCALE, 27 * REPEAT_SCALE);
			if (self.pal == 21)
			{
				self.extra = ELFUZIALTSTR;
			}
			else
			{
				self.extra = ELFUZISTRENGTH;
			}
		}
		else if (self.curAI == 'AIELFUZISEEKING')
		{
			state_elfuziseekstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZIHUNTING')
		{
			state_elfuzihuntstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZIIQ')
		{
			state_elfuziiqstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZISHOOTING')
		{
			state_elfuzishootstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZIJUMPING')
		{
			state_elfuzijumpingstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZIFALLING')
		{
			state_elfuzifallingstate(p, pdist);
		}
		else if (self.curAI == 'AIELFUZIGROW')
		{
			state_genericgrowcode(p, pdist);
		}
		if (self.curAI == 'AIELFUZISHRINKING')
		{
			state_elfuzishrinkstate(p, pdist);
		}
	}
	
}

class NWinterElfUziStayput : DukeActor
{
	default
	{
		pic "ELFUZISTAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AIELFUZISEEKING');
		self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		self.clipdist += 32 * 0.25;
		self.ChangeType('NWinterElfUzi');
	}
	
}

class NWinterSpecBlood : DukeActor
{
	default
	{
		pic "SPECBLOOD";
		action "NONEACTION", 0, 1, 1, 1, 1;
		move "forwardspeed", 1500;
		ai "AIMOVEFORWARD", "NONEACTION", "forwardspeed", faceplayer;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.Scale = (32 * REPEAT_SCALE, 27 * REPEAT_SCALE);
		if (self.curAI == 'none')
		{
			setAI('AIMOVEFORWARD');
		}
		if (self.counter >= 1 && self.sector != null)
		{
			self.spawn('DukeBlood');
			self.spawn('DukeBlood');
			self.spawn('DukeBlood');
			self.spawn('DukeBlood');
			self.killit();
		}
	}
	
}
