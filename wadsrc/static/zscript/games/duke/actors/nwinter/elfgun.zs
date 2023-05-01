
class NWinterElfGun : DukeActor
{
	const ELFGUNSTRENGTH = 75;

	default
	{
		+BADGUY
		+KILLCOUNT
		Strength ELFGUNSTRENGTH;
		
		pic "ELFGUN";
		action "AELFGUNWALK", 0, 4, 5, 1, 20;
		action "AELFGUNRUN", 0, 4, 5, 1, 10;
		action "AELFGUNFROZEN", 0, 1, 5, 1;
		action "AELFGUNGROW", 0, 1, 5, 1;
		action "AELFGUNDYING", 35, 4, 1, 1, 25;
		action "AELFGUNDEAD", 39, 1, 1, 1, 1;
		action "AELFGUNSHOOT", 20, 3, 5, 1, 40;
		move "elfgunwalkspeed", 70;
		move "elfgunrunspeed", 110;
		move "elfgunstopped";
		move "elfgunshrink1", 80;
		ai "AIELFGUNSHRINKING", "AELFGUNWALK", "elfgunshrink1", fleeenemy;
		ai "AIELFGUNDYING", "AELFGUNDYING", "elfgunstopped", faceplayer;
		ai "AIELFGUNSEEKING", "AELFGUNWALK", "elfgunwalkspeed", seekplayer;
		ai "AIELFGUNHUNTING", "AELFGUNRUN", "elfgunrunspeed", faceplayer;
		ai "AIELFGUNSHOOTING", "AELFGUNSHOOT", "elfgunstopped", faceplayer;
		ai "AIELFGUNIQ", "AELFGUNRUN", "elfgunrunspeed", geth| getv;
		ai "AIELFGUNGROW", "AELFGUNGROW", "elfgunstopped", geth| getv;
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfgunseekstate(DukePlayer p, double pdist)
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
					setAI('AIELFGUNHUNTING');
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

	void state_elfgunhuntstate(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pdead))
		{
			setAI('AIELFGUNSEEKING');
		}
		if (self.cansee(p))
		{
			if (pdist < 2000 * maptoworld)
			{
				setAI('AIELFGUNSHOOTING');
				return;
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			if (Duke.rnd(7))
			{
				if (pdist < 8192 * maptoworld)
				{
					setAI('AIELFGUNSHOOTING');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		else
		{
			if (Duke.rnd(48))
			{
				setAI('AIELFGUNIQ');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfguniqstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			setAI('AIELFGUNHUNTING');
		}
		else
		{
			if (Duke.rnd(16))
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIELFGUNSEEKING');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfgunshootstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.checkp(p, pdead))
			{
				setAI('AIELFGUNSEEKING');
			}
			if (self.counter >= 20)
			{
				if (self.counter >= 29)
				{
				}
				else
				{
					if (self.counter >= 28)
					{
						self.PlayActorSound("SHOTGUN_COCK");
					}
				}
			}
			else
			{
				if (self.counter >= 19)
				{
					self.PlayActorSound("SHOTGUN_FIRE");
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
				}
			}
			if (self.actioncounter >= 3)
			{
				self.counter = 0;
				self.actioncounter = 0;
				if (Duke.rnd(64))
				{
					setAI('AIELFGUNHUNTING');
				}
			}
		}
		else
		{
			setAI('AIELFGUNIQ');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfgunhitstate(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.sector != null) self.spawn('DukeBlood');
		state_random_wall_jibs(p, pdist);
		if (self.extra < 0)
		{
			state_random_wall_jibs(p, pdist);
			if (Duke.rnd(128))
			{
				if (self.sector != null) self.spawn('NWinterXmasPresent');
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AIELFGUNGROW');
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('AELFGUNFROZEN');
				self.extra = 0;
				return;
			}
			self.addkill();
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
				setAI('AIELFGUNDYING');
			}
		}
		else
		{
			state_random_wall_jibs(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				self.PlayActorSound("ACTOR_SHRINKING");
				setAI('AIELFGUNSHRINKING');
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			else
			{
				if (Duke.rnd(32))
				{
					setAI('AIELFGUNSHOOTING');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void state_elfgunshrinkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AIELFGUNSEEKING');
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

	void state_elfgundyingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 4)
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
			setAction('AELFGUNDEAD');
			setMove('elfgunstopped', 0);
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
		if (self.curAction.name == 'AELFGUNDEAD')
		{
			if (ud.respawn_monsters) // for enemies:
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = ELFGUNSTRENGTH;
					setAI('AIELFGUNSEEKING');
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
		else if (self.curAction.name == 'AELFGUNFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIELFGUNSEEKING');
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
			if (self.curAI == 'AIELFGUNDYING')
			{
				state_elfgundyingstate(p, pdist);
			}
		}
		if (self.curAI == 'AIELFGUNSHRINKING')
		{
		}
		else
		{
			if (self.curAI == 'AIELFGUNGROW')
			{
			}
			else
			{
				if (self.ifhitbyweapon() >= 0)
				{
					state_elfgunhitstate(p, pdist);
				}
			}
		}
		if (self.curAI == 'none')
		{
			setAI('AIELFGUNSEEKING');
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.clipdist += 32 * 0.25;
			self.Scale = (32 * REPEAT_SCALE, 27 * REPEAT_SCALE);
			self.extra = ELFGUNSTRENGTH;
		}
		else if (self.curAI == 'AIELFGUNSEEKING')
		{
			state_elfgunseekstate(p, pdist);
		}
		else if (self.curAI == 'AIELFGUNHUNTING')
		{
			state_elfgunhuntstate(p, pdist);
		}
		else if (self.curAI == 'AIELFGUNIQ')
		{
			state_elfguniqstate(p, pdist);
		}
		else if (self.curAI == 'AIELFGUNSHOOTING')
		{
			state_elfgunshootstate(p, pdist);
		}
		else if (self.curAI == 'AIELFGUNGROW')
		{
			state_genericgrowcode(p, pdist);
		}
		if (self.curAI == 'AIELFGUNSHRINKING')
		{
			state_elfgunshrinkstate(p, pdist);
		}
	}
	
}


class NWinterElfGunStayput : NWinterElfGun
{
	default
	{
		+BADGUYSTAYPUT
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AIELFGUNSEEKING');
		self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		self.clipdist += 32 * 0.25;
		self.ChangeType('NWinterElfGun');
	}
	
}
