class DukeBossBase : DukeActor
{
	const PIGCOPSTRENGTH = 100;

	default
	{
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+BOSS;
		+ALTHITSCANDIRECTION;

	}

	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner is 'DukeRespawnController')
			self.pal = owner.pal;
		
		if (self.pal != 0 && (!isWorldTour() || !(currentLevel.gameflags & MapRecord.LEVEL_WT_BOSSSPAWN) || self.pal != 22))
		{
			self.clipdist = 20;
			self.scale = (0.625, 0.625);
		}
		else
		{
			self.scale = (1.25, 1.25);
			self.clipdist = 41;
		}
	}
	
}

class DukeBoss1 : DukeBossBase
{
	const BOSS1STRENGTH = 4500;
	const BOSS1PALSTRENGTH = 1000;

	default
	{
		pic "BOSS1";
		+DONTENTERWATER;
		Strength BOSS1STRENGTH;
		
		action "ABOSS1WALK", 0, 4, 5, 1, 12;
		action "ABOSS1FROZEN", 30, 1, 5;
		action "ABOSS1RUN", 0, 6, 5, 1, 5;
		action "ABOSS1SHOOT", 30, 2, 5, 1, 4;
		action "ABOSS1LOB", 40, 2, 5, 1, 35;
		action "ABOSS1DYING", 50, 5, 1, 1, 35;
		action "BOSS1FLINTCH", 50, 1, 1, 1, 1;
		action "ABOSS1DEAD", 55;
		move "PALBOSS1SHRUNKRUNVELS", 32;
		move "PALBOSS1RUNVELS", 128;
		move "BOSS1WALKVELS", 208;
		move "BOSS1RUNVELS", 296;
		move "BOSS1STOPPED";
		ai "AIBOSS1SEEKENEMY", "ABOSS1WALK", "BOSS1WALKVELS", seekplayer;
		ai "AIBOSS1RUNENEMY", "ABOSS1RUN", "BOSS1RUNVELS", faceplayer;
		ai "AIBOSS1SHOOTENEMY", "ABOSS1SHOOT", "BOSS1STOPPED", faceplayer;
		ai "AIBOSS1LOBBED", "ABOSS1LOB", "BOSS1STOPPED", faceplayer;
		ai "AIBOSS1DYING", "ABOSS1DYING", "BOSS1STOPPED", faceplayer;
		ai "AIBOSS1PALSHRINK", "ABOSS1WALK", "PALBOSS1SHRUNKRUNVELS", furthestdir;
		ai "AIBOSS1ONFIRE", "ABOSS1WALK", "BOSS1WALKVELS", fleeenemy;
		
	}
	
	override void PlayFTASound(int mode)
	{
		Duke.PlaySound("BOS1_RECOG");
	}
	
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1onfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AIBOSS1SEEKENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("BOS1_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AIBOSS1DYING');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1palshrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AITROOPSEEKENEMY');
		}
		else if (self.counter >= SHRUNKCOUNT)
		{
			self.actorsizeto(40 * REPEAT_SCALE, 40 * REPEAT_SCALE);
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

	void state_checkboss1seekstate(DukePlayer p, double pdist)
	{
		setAI('AIBOSS1SEEKENEMY');
		if (self.pal == 0)
		{
		}
		else
		{
			setMove('PALBOSS1RUNVELS', seekplayer);
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1runenemystate(DukePlayer p, double pdist)
	{
		if (pdist < 2048 * maptoworld)
		{
			if (self.checkp(p, palive))
			{
				setAI('AIBOSS1SHOOTENEMY');
			}
			return;
		}
		else if (self.cansee(p))
		{
			if (self.actioncounter >= 6)
			{
				if (self.ifcanshoottarget(p, pdist))
				{
					self.actioncounter = 0;
					self.PlayActorSound("BOS1_WALK");
				}
				else
				{
					setAI('AIBOSS1SEEKENEMY');
				}
			}
		}
		else
		{
			setAI('AIBOSS1SEEKENEMY');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1seekenemystate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			self.PlayActorSound("BOS1_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
		else
		{
			if (self.actioncounter >= 6)
			{
				self.actioncounter = 0;
				self.PlayActorSound("BOS1_WALK");
			}
		}
		if (pdist < 2548 * maptoworld)
		{
			if (self.checkp(p, palive))
			{
				setAI('AIBOSS1SHOOTENEMY');
				return;
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.cansee(p))
		{
			if (self.counter >= 32)
			{
				if (Duke.rnd(32))
				{
					if (self.checkp(p, palive))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							setAI('AIBOSS1SHOOTENEMY');
						}
					}
				}
				else
				{
					if (pdist > 2548 * maptoworld)
					{
						if (Duke.rnd(192))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								if (Duke.rnd(64))
								{
									setAI('AIBOSS1RUNENEMY');
									if (self.pal == 0)
									{
									}
									else
									{
										setMove('PALBOSS1RUNVELS', seekplayer);
									}
								}
								else
								{
									setAI('AIBOSS1LOBBED');
								}
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1dyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS1DEAD')
		{
			if (self.pal == 0)
			{
				return;
			}
			if (ud.respawn_monsters) // for enemies:
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = PIGCOPSTRENGTH;
					state_checkboss1seekstate(p, pdist);
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
						self.killit();
					}
				}
				return;
			}
		}
		if (self.actioncounter >= 5)
		{
			if (self.pal == 2)
			{
				self.addkill();
				state_spawnburnedcorpse(p, pdist);
			}
			else
			{
				if (self.floorz - self.pos.Z < 8)
				{
					self.PlayActorSound("THUD");
				}
			}
			setAction('ABOSS1DEAD');
			self.cstat = 0;
			if (self.pal == 0)
			{
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
			}
		}
	}
	void state_boss1lobbedstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.actioncounter >= 2)
			{
				self.actioncounter = 0;
				self.PlayActorSound("BOS1_ATTACK2");
				self.shoot('DukeMortar');
			}
			else
			{
				if (self.counter >= 64)
				{
					if (Duke.rnd(16))
					{
						state_checkboss1seekstate(p, pdist);
					}
				}
			}
		}
		else
		{
			state_checkboss1seekstate(p, pdist);
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1shootenemy(DukePlayer p, double pdist)
	{
		if (self.counter >= 72)
		{
			state_checkboss1seekstate(p, pdist);
		}
		else
		{
			if (self.curAction.name == 'ABOSS1SHOOT')
			{
				if (self.actioncounter >= 2)
				{
					self.PlayActorSound("BOS1_ATTACK1");
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.shoot('DukeShotSpark');
					self.actioncounter = 0;
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkboss1hitstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			if (self.sector != null) self.spawn('DukeBloodPool');
		}
		if (self.extra < 0)
		{
			if (self.pal == 0)
			{
				p.actor.PlayActorSound("PLAYER_TALKTOBOSSFALL", CHAN_AUTO, CHANF_LOCAL);
			}
			else
			{
				if (Duke.rnd(64))
				{
					p.actor.PlayActorSound("PLAYER_TALKTOBOSSFALL", CHAN_AUTO, CHANF_LOCAL);
				}
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.PlayActorSound("SOMETHINGFROZE");
					self.tempval = self.pal;
					self.pal = 1;
					setMove('none', 0);
					setAction('ABOSS1FROZEN');
					self.extra = 0;
					return;
				}
			}
			self.PlayActorSound("BOS1_DYING");
			self.addkill();
			setAI('AIBOSS1DYING');
		}
		else
		{
			if (Duke.rnd(32))
			{
				setAction('BOSS1FLINTCH');
				setMove('none', 0);
			}
			if (self.pal == 0)
			{
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					self.PlayActorSound("ACTOR_SHRINKING");
					setAI('AIBOSS1PALSHRINK');
					self.cstat = 0;
					return;
				}
			}
			self.PlayActorSound("BOS1_PAIN", CHAN_AUTO, CHANF_SINGULAR);
			self.spawndebris(DukeScrap.Scrap1, 1);
			self.spawnguts('DukeJibs6', 1);
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss1code(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS1FROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIBOSS1SEEKENEMY');
				self.tempval = self.pal;
				self.pal = 21;
			}
			else
			{
				if (self.counter >= FROZENDRIPTIME)
				{
					if (self.actioncounter >= 26)
					{
						if (self.sector != null) self.spawn('DukeWaterDrip');
						self.actioncounter = 0;
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
		if (self.curAI == 'none')
		{
			if (self.pal == 0)
			{
				setAI('AIBOSS1RUNENEMY');
			}
			else
			{
				if (self.pal == 21 || !isWorldTour())
				{
					self.extra = BOSS1PALSTRENGTH;
				}
				setAI('AIBOSS1SHOOTENEMY');
			}
		}
		else if (self.curAction.name == 'BOSS1FLINTCH')
		{
			if (self.actioncounter >= 3)
			{
				setAI('AIBOSS1SHOOTENEMY');
			}
		}
		else if (self.curAI == 'AIBOSS1SEEKENEMY')
		{
			state_boss1seekenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS1RUNENEMY')
		{
			state_boss1runenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS1SHOOTENEMY')
		{
			state_boss1shootenemy(p, pdist);
		}
		else if (self.curAI == 'AIBOSS1LOBBED')
		{
			state_boss1lobbedstate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS1PALSHRINK')
		{
			state_boss1palshrunkstate(p, pdist);
		}
		if (self.curAI == 'AIBOSS1DYING')
		{
			state_boss1dyingstate(p, pdist);
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkboss1hitstate(p, pdist);
			}
			else
			{
				if (self.checkp(p, palive))
				{
					if (self.pal == 0)
					{
						if (pdist < 1280 * maptoworld)
						{
							p.addphealth(-1000, false);
							p.pals = color(63, 63, 0, 0);
						}
						if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
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

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		state_boss1code(p, pdist);
	}
	
}


class DukeBoss1Stayput : DukeBoss1
{
	default
	{
		pic "BOSS1STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void PlayFTASound(int mode)
	{
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeBoss1');
	}
	
}
	