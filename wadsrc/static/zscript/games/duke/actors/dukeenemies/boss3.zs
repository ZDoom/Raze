class DukeBoss3 : DukeBossBase
{
	const BOSS3STRENGTH = 4500;
	const BOSS3PALSTRENGTH = 1000;
	
	default
	{
		pic "BOSS3";
		-ALTHITSCANDIRECTION;
		Strength BOSS3STRENGTH;
		
		action "ABOSS3WALK", 0, 4, 5, 1, 30;
		action "ABOSS3FROZEN", 0, 1, 5;
		action "ABOSS3RUN", 0, 4, 5, 1, 15;
		action "ABOSS3LOB", 20, 4, 5, 1, 50;
		action "ABOSS3LOBBING", 30, 2, 5, 1, 15;
		action "ABOSS3DYING", 40, 8, 1, 1, 20;
		action "BOSS3FLINTCH", 40, 1, 1, 1, 1;
		action "ABOSS3DEAD", 48;
		move "PALBOSS3SHRUNKRUNVELS", 32;
		move "PALBOSS3RUNVELS", 84;
		move "BOSS3WALKVELS", 208;
		move "BOSS3RUNVELS", 270;
		move "BOSS3STOPPED";
		ai "AIBOSS3SEEKENEMY", "ABOSS3WALK", "BOSS3WALKVELS", seekplayer;
		ai "AIBOSS3RUNENEMY", "ABOSS3RUN", "BOSS3RUNVELS", faceplayerslow;
		ai "AIBOSS3LOBENEMY", "ABOSS3LOB", "BOSS3STOPPED", faceplayer;
		ai "AIBOSS3DYING", "ABOSS3DYING", "BOSS3STOPPED", faceplayer;
		ai "AIBOSS3PALSHRINK", "ABOSS3WALK", "PALBOSS3SHRUNKRUNVELS", faceplayer;
		
	}
		
	override void PlayFTASound(int mode)
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS3_RECOG");
		else Duke.PlaySound("RIPHEADNECK");
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss3palshrunkstate(DukePlayer p, double pdist)
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

	void state_checkboss3seekstate(DukePlayer p, double pdist)
	{
		setAI('AIBOSS3SEEKENEMY');
		if (self.pal == 0)
		{
		}
		else
		{
			setMove('PALBOSS3RUNVELS', seekplayer);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss3runenemystate(DukePlayer p, double pdist)
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
					setAI('AIBOSS3SEEKENEMY');
				}
			}
		}
		else
		{
			setAI('AIBOSS3SEEKENEMY');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss3seekenemystate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			self.PlayActorSound("BOS3_ROAM", CHAN_AUTO, CHANF_SINGULAR);
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
				if (Duke.rnd(48))
				{
					if (self.ifcanshoottarget(p, pdist))
					{
						if (Duke.rnd(64))
						{
							if (pdist > 4096 * maptoworld)
							{
								setAI('AIBOSS3RUNENEMY');
								if (self.pal == 0)
								{
									return;
								}
								setMove('PALBOSS3RUNVELS', seekplayer);
								return;
							}
							if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
						}
						if (self.checkp(p, palive))
						{
							setAI('AIBOSS3LOBENEMY');
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

	void state_boss3dyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS3DEAD')
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
					state_checkboss3seekstate(p, pdist);
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
			setAction('ABOSS3DEAD');
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

	void state_boss3lobbedstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.curAction.name == 'ABOSS3LOBBING')
			{
				if (self.actioncounter >= 2)
				{
					self.shoot('DukeRPG');
					self.actioncounter = 0;
					if (Duke.rnd(8))
					{
						setAI('AIBOSS3SEEKENEMY');
					}
				}
			}
			if (self.actioncounter >= 3)
			{
				setAction('ABOSS3LOBBING');
				self.counter = 0;
			}
		}
		else
		{
			state_checkboss3seekstate(p, pdist);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkboss3hitstate(DukePlayer p, double pdist)
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
					setAction('ABOSS3FROZEN');
					self.extra = 0;
					return;
				}
			}
			self.addkill();
			setAI('AIBOSS3DYING');
			self.PlayActorSound("BOS3_DYING");
			self.PlayActorSound("JIBBED_ACTOR9");
		}
		else
		{
			if (Duke.rnd(32))
			{
				setAction('BOSS3FLINTCH');
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
					setAI('AIBOSS3PALSHRINK');
					return;
				}
			}
			self.PlayActorSound("BOS3_PAIN", CHAN_AUTO, CHANF_SINGULAR);
			self.spawndebris(DukeScrap.Scrap1, 1);
			self.spawnguts('DukeJibs6', 1);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss3code(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS3FROZEN')
		{
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
				setAI('AIBOSS3RUNENEMY');
			}
			else
			{
				if (self.pal == 21 || !isWorldTour())
				{
					self.extra = BOSS3PALSTRENGTH;
				}
				setAI('AIBOSS3LOBENEMY');
			}
		}
		else if (self.curAction.name == 'BOSS3FLINTCH')
		{
			if (self.actioncounter >= 3)
			{
				setAI('AIBOSS3SEEKENEMY');
			}
		}
		else if (self.curAI == 'AIBOSS3SEEKENEMY')
		{
			state_boss3seekenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS3RUNENEMY')
		{
			state_boss3runenemystate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS3LOBENEMY')
		{
			state_boss3lobbedstate(p, pdist);
		}
		else if (self.curAI == 'AIBOSS3PALSHRINK')
		{
			state_boss3palshrunkstate(p, pdist);
		}
		if (self.curAI == 'AIBOSS3DYING')
		{
			state_boss3dyingstate(p, pdist);
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkboss3hitstate(p, pdist);
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
		state_boss3code(p, pdist);
	}

}


class DukeBoss3Stayput : DukeBoss3
{
	default
	{
		pic "BOSS3STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeBoss2');
	}
	
}
	
