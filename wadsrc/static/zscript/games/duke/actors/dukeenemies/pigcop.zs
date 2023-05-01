class DukePigCop : DukeActor
{
	const PIGCOPSTRENGTH = 100;
	
	default
	{
		pic "PIGCOP";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		Strength PIGCOPSTRENGTH;
		startAction "APIGSTAND";
		
		action "APIGWALK", 0, 4, 5, 1, 20;
		action "APIGRUN", 0, 4, 5, 1, 11;
		action "APIGSHOOT", 30, 2, 5, 1, 58;
		action "APIGCOCK", 25, 1, 5, 1, 16;
		action "APIGSTAND", 30, 1, 5, 1, 1;
		action "APIGDIVE", 40, 2, 5, 1, 40;
		action "APIGDIVESHOOT", 45, 2, 5, 1, 58;
		action "APIGDYING", 55, 5, 1, 1, 15;
		action "APIGHIT", 55, 1, 1, 1, 10;
		action "APIGDEAD", 60, 1, 1, 1, 1;
		action "APIGFROZEN", 0, 1, 5;
		action "APIGGROW", 0;
		move "PIGWALKVELS", 72;
		move "PIGRUNVELS", 108;
		move "PIGSTOPPED";
		ai "AIPIGSEEKENEMY", "APIGWALK", "PIGWALKVELS", seekplayer;
		ai "AIPIGSHOOTENEMY", "APIGSHOOT", "PIGSTOPPED", faceplayer;
		ai "AIPIGFLEEENEMY", "APIGWALK", "PIGWALKVELS", fleeenemy;
		ai "AIPIGSHOOT", "APIGSHOOT", "PIGSTOPPED", faceplayer;
		ai "AIPIGDODGE", "APIGRUN", "PIGRUNVELS", dodgebullet;
		ai "AIPIGCHARGE", "APIGRUN", "PIGRUNVELS", seekplayer;
		ai "AIPIGDIVING", "APIGDIVE", "PIGSTOPPED", faceplayer;
		ai "AIPIGDYING", "APIGDYING", "PIGSTOPPED", faceplayer;
		ai "AIPIGSHRINK", "APIGWALK", "SHRUNKVELS", fleeenemy;
		ai "AIPIGGROW", "APIGGROW", "PIGSTOPPED", faceplayerslow;
		ai "AIPIGHIT", "APIGHIT", "PIGSTOPPED", faceplayer;
		ai "AIPIGONFIRE", "APIGWALK", "PIGWALKVELS", fleeenemy;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("PIG_RECOG");
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_pigonfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AIPIGSEEKENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("PIG_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AIPIGDYING');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_pigseekenemystate(DukePlayer p, double pdist)
	{
		if (self.curAI == 'AIPIGCHARGE')
		{
			if (self.cansee(p))
			{
				if (pdist < 3084 * maptoworld)
				{
					if (self.movflag > kHitSector)
					{
						setAI('AIPIGSEEKENEMY');
					}
					else
					{
						setAI('AIPIGDIVING');
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			return;
		}
		else
		{
			if (self.floorz - self.pos.Z < 32)
			{
				if (pdist > 4096 * maptoworld)
				{
					if (self.actorstayput == null)
					{
						setAI('AIPIGCHARGE');
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				if (Duke.rnd(8))
				{
					if (self.dodge() == 1)
					{
						setAI('AIPIGDODGE');
					}
				}
			}
		}
		if (Duke.rnd(128))
		{
			if (self.cansee(p))
			{
				if (self.curAI == 'AIPIGDODGE')
				{
					if (self.counter >= 32)
					{
						setAI('AIPIGCHARGE');
					}
					return;
				}
				if (self.floorz - self.pos.Z < 32)
				{
					if (pdist < 1024 * maptoworld)
					{
						if (self.checkp(p, palive))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								setAI('AIPIGSHOOTENEMY');
								return;
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
					if (self.counter >= 48)
					{
						if (Duke.rnd(8))
						{
							if (self.checkp(p, palive))
							{
								if (self.ifcanshoottarget(p, pdist))
								{
									if (Duke.rnd(192))
									{
										setAI('AIPIGSHOOTENEMY');
									}
									else
									{
										setAI('AIPIGDIVING');
									}
								}
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

	void state_pigshootenemystate(DukePlayer p, double pdist)
	{
		if (self.counter >= 12)
		{
		}
		else
		{
			if (self.counter >= 11)
			{
				if (self.ifcanshoottarget(p, pdist))
				{
					self.PlayActorSound("PIG_ATTACK");
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
				}
				else
				{
					setAI('AIPIGSEEKENEMY');
				}
			}
		}
		if (self.counter >= 25)
		{
		}
		else
		{
			if (self.counter >= 24)
			{
				setAction('APIGCOCK');
				self.PlayActorSound("SHOTGUN_COCK");
			}
		}
		if (self.counter >= 48)
		{
		}
		else
		{
			if (self.counter >= 47)
			{
				if (self.ifcanshoottarget(p, pdist))
				{
					self.PlayActorSound("PIG_ATTACK");
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
					self.shoot('DukeShotgunShot');
				}
				else
				{
					setAI('AIPIGSEEKENEMY');
				}
			}
		}
		if (self.counter >= 60)
		{
		}
		else
		{
			if (self.counter >= 59)
			{
				setAction('APIGCOCK');
				self.PlayActorSound("SHOTGUN_COCK");
			}
		}
		if (self.counter >= 72)
		{
			if (Duke.rnd(64))
			{
				self.counter = 0;
			}
			else
			{
				if (pdist < 768 * maptoworld)
				{
					setAI('AIPIGFLEEENEMY');
				}
				else
				{
					setAI('AIPIGSEEKENEMY');
				}
			}
		}
		if (self.curAction.name == 'APIGCOCK')
		{
			if (self.actioncounter >= 2)
			{
				setAction('APIGSHOOT');
			}
		}
		else
		{
			setAI('AIPIGSEEKENEMY');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_pigfleeenemystate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 8)
		{
			setAI('AIPIGSEEKENEMY');
		}
		else
		{
			if (self.movflag > kHitSector)
			{
				setAI('AIPIGSEEKENEMY');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_pigdivestate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'APIGDIVESHOOT')
		{
			if (self.cansee(p))
			{
				if (self.counter >= 12)
				{
				}
				else
				{
					if (self.counter >= 11)
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							self.PlayActorSound("PIG_ATTACK");
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
						}
						else
						{
							setAI('AIPIGSEEKENEMY');
						}
					}
				}
				if (self.counter >= 25)
				{
				}
				else
				{
					if (self.counter >= 24)
					{
						self.PlayActorSound("SHOTGUN_COCK");
					}
				}
				if (self.counter >= 48)
				{
				}
				else
				{
					if (self.counter >= 47)
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							self.PlayActorSound("PIG_ATTACK");
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
							self.shoot('DukeShotgunShot');
						}
						else
						{
							setAI('AIPIGSEEKENEMY');
						}
					}
				}
				if (self.counter >= 60)
				{
				}
				else
				{
					if (self.counter >= 59)
					{
						self.PlayActorSound("SHOTGUN_COCK");
						if (self.floorz - self.ceilingz < 32)
						{
							setAI('AIPIGDIVING');
						}
						else
						{
							if (pdist < 4096 * maptoworld)
							{
								setAI('AIPIGFLEEENEMY');
							}
							else
							{
								setAI('AIPIGSEEKENEMY');
							}
						}
					}
				}
			}
			else if (self.floorz - self.ceilingz < 32)
			{
				setAI('AIPIGDIVING');
			}
			else
			{
				setAI('AIPIGSEEKENEMY');
			}
		}
		else
		{
			if (self.actioncounter >= 2)
			{
				if (self.checkp(p, palive))
				{
					self.counter = 0;
					setAction('APIGDIVESHOOT');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkpighitstate(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.extra < 0)
		{
			state_random_wall_jibs(p, pdist);
			if (Duke.rnd(16))
			{
				if (self.sector != null) self.spawn('DukeShield');
			}
			else
			{
				state_drop_shotgun(p, pdist);
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AIPIGGROW');
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('APIGFROZEN');
				self.extra = 0;
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				state_spawnburnedcorpse(p, pdist);
			}
			else
			{
				setAI('AIPIGDYING');
			}
			self.PlayActorSound("PIG_DYING");
		}
		else
		{
			if (self.attackertype.GetClassName() != 'DukeFlamethrowerFlame')
			{
				self.PlayActorSound("PIG_PAIN");
				state_random_wall_jibs(p, pdist);
			}
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				if (self.pal == 2)
				{
					self.pal = self.tempval;
					self.tempval = 0;
				}
				self.PlayActorSound("ACTOR_SHRINKING");
				setAI('AIPIGSHRINK');
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				if (self.pal == 2)
				{
				}
				else
				{
					self.tempval = self.pal;
					self.pal = 2;
					setAI('AIPIGONFIRE');
					self.counter = 0;
					state_pigonfirestate(p, pdist);
				}
			}
			else if (self.curAI == 'AIPIGONFIRE')
			{
				return;
			}
			else if (Duke.rnd(isVacation() || !isPlutoPak() ? 64 : 32)) // This changed between 1.3 and 1.5 - Vaca retains the old value even for its 1.5 version
			{
				setAI('AIPIGHIT');
			}
			else if (Duke.rnd(64))
			{
				setAI('AIPIGSHOOTENEMY');
			}
			else if (Duke.rnd(64))
			{
				setAI('AIPIGDIVING');
				setAction('APIGDIVESHOOT');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_pigshrinkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AIPIGSEEKENEMY');
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

	void state_pigdyingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 5)
		{
			if (self.pal == 2)
			{
				self.addkill();
				state_spawnburnedcorpse(p, pdist);
			}
			else
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
			}
			setAction('APIGDEAD');
			setMove('PIGSTOPPED', 0);
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
		if (self.curAction.name == 'APIGSTAND')
		{
			setAI('AIPIGSEEKENEMY');
		}
		else if (self.curAction.name == 'APIGDEAD')
		{
			if (ud.respawn_monsters) // for enemies:
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = PIGCOPSTRENGTH;
					setAI('AIPIGSEEKENEMY');
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
		else if (self.curAction.name == 'APIGFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIPIGSEEKENEMY');
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
		}
		else if (self.curAI == 'AIPIGDYING')
		{
			state_pigdyingstate(p, pdist);
		}
		else if (self.curAI == 'AIPIGHIT')
		{
			if (self.actioncounter >= 3)
			{
				setAI('AIPIGSEEKENEMY');
			}
		}
		else if (self.curAI == 'AIPIGSHRINK')
		{
			state_pigshrinkstate(p, pdist);
		}
		else
		{
			if (self.curAI == 'AIPIGSEEKENEMY')
			{
				state_pigseekenemystate(p, pdist);
			}
			else if (self.curAI == 'AIPIGDODGE')
			{
				state_pigseekenemystate(p, pdist);
			}
			else if (self.curAI == 'AIPIGSHOOTENEMY')
			{
				state_pigshootenemystate(p, pdist);
			}
			else if (self.curAI == 'AIPIGGROW')
			{
				state_genericgrowcode(p, pdist);
			}
			else if (self.curAI == 'AIPIGFLEEENEMY')
			{
				state_pigfleeenemystate(p, pdist);
			}
			else if (self.curAI == 'AIPIGDIVING')
			{
				state_pigdivestate(p, pdist);
			}
			else if (self.curAI == 'AIPIGCHARGE')
			{
				state_pigseekenemystate(p, pdist);
			}
			else if (self.curAI == 'AIPIGONFIRE')
			{
				state_pigonfirestate(p, pdist);
			}
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkpighitstate(p, pdist);
			}
			if (Duke.rnd(1))
			{
				if (Duke.rnd(32))
				{
					self.PlayActorSound("PIG_ROAM", CHAN_AUTO, CHANF_SINGULAR);
				}
				else if (Duke.rnd(64))
				{
					self.PlayActorSound("PIG_ROAM2", CHAN_AUTO, CHANF_SINGULAR);
				}
				else
				{
					self.PlayActorSound("PIG_ROAM3", CHAN_AUTO, CHANF_SINGULAR);
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

class DukePigCopStayput: DukePigCop
{
	default
	{
		pic "PIGCOPSTAYPUT";
		+BADGUYSTAYPUT;
		startAction "none";
	}
		
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AIPIGSEEKENEMY');
		self.ChangeType('DukePigCop');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukePigCopDive : DukePigCopStayput
{
	default
	{
		pic "PIGCOPDIVE";
	}
	
	override void PlayFTASound(int mode)
	{
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AIPIGDIVING');
		setAction('APIGDIVESHOOT');
		self.ChangeType('DukePigCop');
	}
}

