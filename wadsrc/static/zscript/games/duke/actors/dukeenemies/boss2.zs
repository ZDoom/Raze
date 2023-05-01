class DukeBoss2 : DukeBossBase
{
	const BOSS2STRENGTH = 4500;
	const BOSS2PALSTRENGTH = 1000;

	default
	{
		pic "BOSS2";
		-ALTHITSCANDIRECTION;
		+NONSMOKYROCKET; // If this wasn't needed for a CON defined actor it could be handled better
		+SPECIALINIT;
		+ST3CONFINED;
		+DONTENTERWATER;
		Strength BOSS2STRENGTH;

		action "ABOSS2WALK", 0, 4, 5, 1, 30;
		action "ABOSS2FROZEN", 0, 1, 5;
		action "ABOSS2RUN", 0, 4, 5, 1, 15;
		action "ABOSS2SHOOT", 20, 2, 5, 1, 15;
		action "ABOSS2LOB", 30, 2, 5, 1, 105;
		action "ABOSS2DYING", 40, 8, 1, 1, 35;
		action "BOSS2FLINTCH", 40, 1, 1, 1, 1;
		action "ABOSS2DEAD", 48;
		move "PALBOSS2SHRUNKRUNVELS", 32;
		move "PALBOSS2RUNVELS", 84;
		move "BOSS2WALKVELS", 192;
		move "BOSS2RUNVELS", 256;
		move "BOSS2STOPPED";
		ai "AIBOSS2SEEKENEMY", "ABOSS2WALK", "BOSS2WALKVELS", seekplayer;
		ai "AIBOSS2RUNENEMY", "ABOSS2RUN", "BOSS2RUNVELS", faceplayer;
		ai "AIBOSS2SHOOTENEMY", "ABOSS2SHOOT", "BOSS2STOPPED", faceplayer;
		ai "AIBOSS2LOBBED", "ABOSS2LOB", "BOSS2STOPPED", faceplayer;
		ai "AIBOSS2DYING", "ABOSS2DYING", "BOSS2STOPPED", faceplayer;
		ai "AIBOSS2PALSHRINK", "ABOSS2WALK", "PALBOSS2SHRUNKRUNVELS", furthestdir;
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void PlayFTASound(int mode)
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS2_RECOG");
		else Duke.PlaySound("WHIPYOURASS");
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2palshrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
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

	void state_checkboss2seekstate(DukePlayer p, double pdist)
	{
		setAI('AIBOSS2SEEKENEMY');
		if (self.pal == 0)
		{
		}
		else
		{
			setMove('PALBOSS2RUNVELS', seekplayer);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2runenemystate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.actioncounter >= 3)
			{
				if (self.ifcanshoottarget(p, pdist))
				{
					self.actioncounter = 0;
					self.PlayActorSound("BOS1_WALK");
				}
				else
				{
					setAI('AIBOSS2SEEKENEMY');
				}
			}
			if (self.counter >= 48)
			{
				if (Duke.rnd(2))
				{
					if (self.checkp(p, palive))
					{
						self.PlayActorSound("BOS2_ATTACK");
						setAI('AIBOSS2SHOOTENEMY');
					}
				}
			}
		}
		else
		{
			setAI('AIBOSS2SEEKENEMY');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2seekenemystate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			self.PlayActorSound("BOS2_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
		else
		{
			if (self.actioncounter >= 3)
			{
				self.actioncounter = 0;
				self.PlayActorSound("BOS1_WALK");
			}
		}
		if (self.cansee(p))
		{
			if (self.counter >= 32)
			{
				if (self.checkp(p, palive))
				{
					if (Duke.rnd(48))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							if (Duke.rnd(64))
							{
								if (pdist > 4096 * maptoworld)
								{
									setAI('AIBOSS2RUNENEMY');
									if (self.pal != 0)
									{
										setMove('PALBOSS2RUNVELS', seekplayer);
									}
									return;
								}
								if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
							}
							if (pdist < 10240 * maptoworld)
							{
								if (Duke.rnd(128))
								{
									self.PlayActorSound("BOS2_ATTACK");
									setAI('AIBOSS2LOBBED');
								}
							}
							else
							{
								self.PlayActorSound("BOS2_ATTACK");
								setAI('AIBOSS2SHOOTENEMY');
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

	void state_boss2dyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS2DEAD')
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
					state_checkboss2seekstate(p, pdist);
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
		if (self.actioncounter >= 8)
		{
			if (self.floorz - self.pos.Z < 8)
			{
				self.PlayActorSound("THUD");
			}
			setAction('ABOSS2DEAD');
			self.cstat = 0;
			if (self.pal == 0)
			{
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2lobbedstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.actioncounter >= 2)
			{
				self.actioncounter = 0;
			}
			else if (self.actioncounter >= 1)
			{
				if (Duke.rnd(128))
				{
					self.shoot('DukeCoolExplosion1');
				}
			}
			else
			{
				if (self.counter >= 64)
				{
					if (Duke.rnd(16))
					{
						state_checkboss2seekstate(p, pdist);
					}
				}
			}
		}
		else
		{
			state_checkboss2seekstate(p, pdist);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2shootenemy(DukePlayer p, double pdist)
	{
		if (self.counter >= 72)
		{
			state_checkboss2seekstate(p, pdist);
		}
		else
		{
			if (self.curAction.name == 'ABOSS2SHOOT')
			{
				if (self.actioncounter >= 2)
				{
					self.shoot('DukeRPG');
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

	void state_checkboss2hitstate(DukePlayer p, double pdist)
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
					setAction('ABOSS2FROZEN');
					self.extra = 0;
					return;
				}
			}
			self.PlayActorSound("BOS2_DYING");
			self.addkill();
			setAI('AIBOSS2DYING');
		}
		else
		{
			if (Duke.rnd(144))
			{
				if (Duke.rnd(32))
				{
					setAction('BOSS2FLINTCH');
					setMove('none', 0);
				}
				else
				{
					self.PlayActorSound("BOS2_ATTACK");
					setAI('AIBOSS2SHOOTENEMY');
				}
			}
			if (self.pal == 0)
			{
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					self.PlayActorSound("ACTOR_SHRINKING");
					setAI('AIBOSS2PALSHRINK');
					return;
				}
			}
			self.PlayActorSound("BOS2_PAIN", CHAN_AUTO, CHANF_SINGULAR);
			self.spawndebris(DukeScrap.Scrap1, 1);
			self.spawnguts('DukeJibs6', 1);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss2code(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS2FROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIBOSS2SEEKENEMY');
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
				}
				else
				{
					self.addkill();
					self.spriteglass(30);
					self.PlayActorSound("GLASS_BREAKING");
					if (Duke.rnd(84))
					{
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					self.killit();
				}
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
				setAI('AIBOSS2RUNENEMY');
			}
			else
			{
				if (self.pal == 21 || !isWorldTour())
				{
					self.extra = BOSS2PALSTRENGTH;
				}
				self.PlayActorSound("BOS2_ATTACK");
				setAI('AIBOSS2SHOOTENEMY');
			}
		}
		else if (self.curAction.name == 'BOSS2FLINTCH')
		{
			if (self.actioncounter >= 3)
			{
				setAI('AIBOSS2SEEKENEMY');
			}
		}
		else if (self.curAI == 'AIBOSS2SEEKENEMY')
		{
			state_boss2seekenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS2RUNENEMY')
		{
			state_boss2runenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS2SHOOTENEMY')
		{
			state_boss2shootenemy(p, pdist);
		}
		else if (self.curAI == 'AIBOSS2LOBBED')
		{
			state_boss2lobbedstate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS2PALSHRINK')
		{
			state_boss2palshrunkstate(p, pdist);
		}
		if (self.curAI == 'AIBOSS2DYING')
		{
			state_boss2dyingstate(p, pdist);
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkboss2hitstate(p, pdist);
			}
			else
			{
				if (self.checkp(p, palive))
				{
					if (self.pal == 0)
					{
						if (pdist < 1280 * maptoworld)
						{
							p.addphealth(-1000, self.bBIGHEALTH);
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
		state_boss2code(p, pdist);
	}

}


class DukeBoss2Stayput : DukeBoss2
{
	default
	{
		pic "BOSS2STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeBoss2');
	}
	
}
	
