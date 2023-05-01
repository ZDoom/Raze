class DukeNewBeast : DukeActor
{
	const NEWBEASTSTRENGTH = 300;
	const NEWBEASTSCRATCHAMOUNT = -22;
	
	default
	{
		pic "NEWBEAST";
		Strength NEWBEASTSTRENGTH;
		+BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+GREENBLOOD;
		
		action "ANEWBEASTSTAND", 0;
		action "ANEWBEASTWALKING", 10, 4, 5, 1, 12;
		action "ANEWBEASTRUNNING", 10, 4, 5, 1, 8;
		action "ANEWBEASTTHINK", 0, 2, 5, 1, 40;
		action "ANEWBEASTSCRATCHING", 30, 3, 5, 1, 20;
		action "ANEWBEASTDYING", 72, 8, 1, 1, 15;
		action "ANEWBEASTFLINTCH", 71, 1, 1, 1, 1;
		action "ANEWBEASTLYINGDEAD", 79, 1, 1;
		action "ANEWBEASTSCREAM", 50, 2, 5, 1, 40;
		action "ANEWBEASTJUMP", 80, 2, 5, 1, 50;
		action "ANEWBEASTFALL", 90, 1, 5;
		action "ANEWBEASTFROZEN", 10, 1, 5;
		action "ANEWBEASTHANG", 0, 1, 5;
		action "ANEWBEASTHANGDEAD", -1, 1, 5;
		move "NEWBEASTWALKVEL", 182;
		move "NEWBEASTRUNVEL", 256;
		move "NEWBEASTJUMPVEL", 264;
		move "NEWBEASTSTOP";
		ai "AINEWBEASTGETENEMY", "ANEWBEASTWALKING", "NEWBEASTWALKVEL", seekplayer;
		ai "AINEWBEASTDODGE", "ANEWBEASTRUNNING", "NEWBEASTRUNVEL", dodgebullet;
		ai "AINEWBEASTCHARGEENEMY", "ANEWBEASTRUNNING", "NEWBEASTRUNVEL", seekplayer;
		ai "AINEWBEASTFLEENEMY", "ANEWBEASTWALKING", "NEWBEASTWALKVEL", fleeenemy;
		ai "AINEWBEASTSCRATCHENEMY", "ANEWBEASTSCRATCHING", "NEWBEASTSTOP", faceplayerslow;
		ai "AINEWBEASTJUMPENEMY", "ANEWBEASTJUMP", "NEWBEASTJUMPVEL", jumptoplayer;
		ai "AINEWBEASTTHINK", "ANEWBEASTTHINK", "NEWBEASTSTOP";
		ai "AINEWBEASTGROW", "ANEWBEASTSTAND", "NEWBEASTSTOP", faceplayerslow;
		ai "AINEWBEASTDYING", "ANEWBEASTDYING", "NEWBEASTSTOP", faceplayer;
		ai "AINEWBEASTSHOOT", "ANEWBEASTSCREAM", "NEWBEASTSTOP", faceplayerslow;
		ai "AINEWBEASTFLINTCH", "ANEWBEASTFLINTCH", "NEWBEASTSTOP", faceplayerslow;
		ai "AINEWBEASTONFIRE", "ANEWBEASTWALKING", "NEWBEASTWALKVEL", fleeenemy;
		
	}
	

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeast_body_jibs(DukePlayer p, double pdist)
	{
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeLizmanHead', 1);
		}
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeLizmanLeg', 2);
		}
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeLizmanArm', 1);
		}
		if (Duke.rnd(48))
		{
			if (self.sector != null) self.spawn('DukeBloodPool');
		}
	}
	
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeastonfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AINEWBEASTGETENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("NEWBEAST_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AINEWBEASTDYING');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeastseekstate(DukePlayer p, double pdist)
	{
		if (self.actorstayput == null)
		{
			if (self.checkp(p, palive))
			{
				if (self.cansee(p))
				{
					if (pdist < 1596 * maptoworld)
					{
						setAI('AINEWBEASTSCRATCHENEMY');
						return;
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			if (self.curAI == 'AINEWBEASTCHARGEENEMY')
			{
				if (self.checkp(p, palive))
				{
					if (pdist < 1596 * maptoworld)
					{
						if (self.cansee(p))
						{
							setAI('AINEWBEASTSCRATCHENEMY');
							return;
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				if (Duke.rnd(1))
				{
					if (pdist > 4096 * maptoworld)
					{
						if (self.checkp(p, palive))
						{
							if (self.cansee(p))
							{
								setAI('AINEWBEASTSHOOT');
								return;
							}
						}
					}
				}
			}
			else
			{
				if (pdist > 4096 * maptoworld)
				{
					setAI('AINEWBEASTCHARGEENEMY');
					return;
				}
			}
			if (self.floorz - self.pos.Z < 16)
			{
				if (self.counter >= 32)
				{
					if (Duke.rnd(16))
					{
						if (self.pos.Z - self.ceilingz < 96)
						{
							return;
						}
						setAI('AINEWBEASTJUMPENEMY');
					}
				}
				return;
			}
			if (Duke.rnd(4))
			{
				if (self.movflag > kHitSector)
				{
					self.actoroperate();
				}
			}
			else
			{
				if (Duke.rnd(16))
				{
					if (self.dodge() == 1)
					{
						if (self.floorz - self.ceilingz < 128)
						{
							setAI('AINEWBEASTDODGE');
						}
						else
						{
							if (self.actorstayput == null)
							{
								if (Duke.rnd(128))
								{
									if (self.pos.Z - self.ceilingz < 96)
									{
										return;
									}
									setAI('AINEWBEASTJUMPENEMY');
								}
								else
								{
									setAI('AINEWBEASTDODGE');
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if (self.actioncounter >= 16)
			{
				if (self.checkp(p, palive))
				{
					if (pdist < 1596 * maptoworld)
					{
						if (self.cansee(p))
						{
							setAI('AINEWBEASTSCRATCHENEMY');
							return;
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			if (self.counter >= 16)
			{
				if (Duke.rnd(32))
				{
					setMove('NEWBEASTWALKVEL', randomangle | geth);
				}
			}
		}
	}
	void state_newbeastfleestate(DukePlayer p, double pdist)
	{
		if (self.counter >= 8)
		{
			if (Duke.rnd(64))
			{
				if (pdist > 3500 * maptoworld)
				{
					if (self.checkp(p, palive))
					{
						if (self.cansee(p))
						{
							setAI('AINEWBEASTSHOOT');
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		else
		{
			if (self.floorz - self.pos.Z < 16)
			{
				if (self.movflag > kHitSector)
				{
					setAI('AINEWBEASTGETENEMY');
				}
			}
			else
			{
				setAI('AINEWBEASTGETENEMY');
			}
			return;
		}
	}
	void state_newbeastthinkstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(8))
		{
			self.PlayActorSound("NEWBEAST_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
		if (self.actioncounter >= 3)
		{
			if (Duke.rnd(128))
			{
				if (pdist > 3500 * maptoworld)
				{
					if (self.checkp(p, palive))
					{
						if (self.cansee(p))
						{
							setAI('AINEWBEASTSHOOT');
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			else
			{
				setAI('AINEWBEASTGETENEMY');
			}
		}
		if (Duke.rnd(16))
		{
			if (self.dodge() == 1)
			{
				if (self.floorz - self.ceilingz < 96)
				{
					setAI('AINEWBEASTDODGE');
				}
				else
				{
					if (Duke.rnd(128))
					{
						if (self.pos.Z - self.ceilingz < 144)
						{
							return;
						}
						setAI('AINEWBEASTJUMPENEMY');
					}
					else
					{
						setAI('AINEWBEASTDODGE');
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

	void state_newbeastscratchstate(DukePlayer p, double pdist)
	{
		if (self.counter >= 20)
		{
			if (Duke.rnd(8))
			{
				if (self.cansee(p))
				{
					if (pdist < 2048 * maptoworld)
					{
						if (Duke.rnd(128))
						{
							setAI('AINEWBEASTFLEENEMY');
						}
						return;
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				if (Duke.rnd(80))
				{
					setAI('AINEWBEASTTHINK');
				}
				else
				{
					setAI('AINEWBEASTGETENEMY');
				}
			}
		}
		if (self.actioncounter >= 3)
		{
			if (pdist > 1596 * maptoworld)
			{
				setAI('AINEWBEASTTHINK');
			}
			else
			{
				if (self.checkp(p, palive))
				{
					if (self.cansee(p))
					{
						p.pals = color(16, 16, 0, 0);
						p.addphealth(NEWBEASTSCRATCHAMOUNT, self.bBIGHEALTH);
						self.PlayActorSound("PLAYER_GRUNT");
						self.actioncounter = 0;
						self.counter = 0;
					}
				}
			}
		}
		else
		{
			if (self.counter == 14)
			{
				if (pdist < 1596 * maptoworld)
				{
					self.PlayActorSound("NEWBEAST_ATTACK", CHAN_AUTO, CHANF_SINGULAR);
				}
				else
				{
					self.PlayActorSound("NEWBEAST_ATTACKMISS", CHAN_AUTO, CHANF_SINGULAR);
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checknewbeasthit(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.extra < 0)
		{
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('ANEWBEASTFROZEN');
				self.extra = 0;
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.cstat = 0;
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AINEWBEASTGROW');
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.PlayActorSound("SQUISHED");
				state_newbeast_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.PlayActorSound("SQUISHED");
				state_newbeast_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else
			{
				state_rf(p, pdist);
				setAI('AINEWBEASTDYING');
				if (Duke.rnd(64))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
			}
			self.PlayActorSound("NEWBEAST_DYING");
		}
		else
		{
			if (self.attackertype.GetClassName() != 'DukeFlamethrowerFlame')
			{
				self.PlayActorSound("NEWBEAST_PAIN");
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				if (self.pal == 2)
				{
				}
				else
				{
					self.pal = self.tempval;
					self.tempval = 0;
					self.tempval = self.pal;
					self.pal = 2;
					setAI('AINEWBEASTONFIRE');
					self.counter = 0;
					state_newbeastonfirestate(p, pdist);
				}
			}
			else if (self.curAI == 'AINEWBEASTONFIRE')
			{
				return;
			}
			else
			{
				state_random_wall_jibs(p, pdist);
				if (Duke.rnd(32))
				{
					setAI('AINEWBEASTFLINTCH');
				}
				else
				{
					if (Duke.rnd(32))
					{
						if (pdist > 3500 * maptoworld)
						{
							if (self.checkp(p, palive))
							{
								if (self.cansee(p))
								{
									setAI('AINEWBEASTSHOOT');
								}
							}
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

	void state_newbeastjumpstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ANEWBEASTFALL')
		{
			if (self.floorz - self.pos.Z < 16)
			{
				setAI('AINEWBEASTGETENEMY');
			}
		}
		else
		{
			if (self.counter >= 32)
			{
				setAction('ANEWBEASTFALL');
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeastdyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ANEWBEASTLYINGDEAD')
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
			if (self.counter >= RESPAWNACTORTIME)
			{
				if (ud.respawn_monsters) // for enemies:
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = NEWBEASTSTRENGTH;
					setAI('AINEWBEASTGETENEMY');
				}
			}
		}
		else
		{
			if (self.curAI == 'AINEWBEASTDYING')
			{
				if (self.actioncounter >= 7)
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
					setMove('NEWBEASTSTOP', 0);
					setAction('ANEWBEASTLYINGDEAD');
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeastdodgestate(DukePlayer p, double pdist)
	{
		if (self.counter >= 13)
		{
			setAI('AINEWBEASTGETENEMY');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_newbeastcode(DukePlayer p, double pdist)
	{
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			setAI('AINEWBEASTGETENEMY');
		}
		else if (self.curAction.name == 'ANEWBEASTLYINGDEAD')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			state_newbeastdyingstate(p, pdist);
			return;
		}
		else if (self.curAction.name == 'ANEWBEASTFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AINEWBEASTGETENEMY');
				self.tempval = self.pal;
				self.pal = 0;
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
				if (Duke.rnd(84))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
				self.spriteglass(30);
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
		else if (self.curAI == 'AINEWBEASTJUMPENEMY')
		{
			state_newbeastjumpstate(p, pdist);
		}
		else
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			if (self.curAI == 'AINEWBEASTGETENEMY')
			{
				state_newbeastseekstate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTCHARGEENEMY')
			{
				state_newbeastseekstate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTFLINTCH')
			{
				if (self.counter >= 8)
				{
					setAI('AINEWBEASTGETENEMY');
				}
			}
			else if (self.curAI == 'AINEWBEASTDODGE')
			{
				state_newbeastdodgestate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTSCRATCHENEMY')
			{
				state_newbeastscratchstate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTFLEENEMY')
			{
				state_newbeastfleestate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTTHINK')
			{
				state_newbeastthinkstate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTGROW')
			{
				state_genericgrowcode(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTDYING')
			{
				state_newbeastdyingstate(p, pdist);
			}
			else if (self.curAI == 'AINEWBEASTSHOOT')
			{
				if (self.checkp(p, pshrunk))
				{
					setAI('AINEWBEASTGETENEMY');
				}
				else if (self.counter >= 26)
				{
					setAI('AINEWBEASTGETENEMY');
				}
				else if (self.counter >= 25)
				{
					self.shoot('DukeShrinker');
				}
				else if (self.counter == 4)
				{
					self.PlayActorSound("NEWBEAST_SPIT");
				}
			}
			else
			{
				if (self.curAI == 'AINEWBEASTONFIRE')
				{
					state_newbeastonfirestate(p, pdist);
				}
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_checknewbeasthit(p, pdist);
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
		if (self.curAction.name == 'none')
		{
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.Scale = (40 * REPEAT_SCALE, 40 * REPEAT_SCALE);
			setAI('AINEWBEASTDODGE');
		}
		if (self.curAction.name == 'ANEWBEASTFROZEN')
		{
			state_newbeastcode(p, pdist);
		}
		else
		{
			if (self.pal == 2)
			{
			}
			else
			{
				self.tempval = self.pal;
				self.pal = 6;
			}
			state_newbeastcode(p, pdist);
			if (self.curAction.name == 'ANEWBEASTFROZEN')
			{
				return;
			}
			if (self.pal == 2)
			{
			}
			else
			{
				self.pal = self.tempval;
				self.tempval = 0;
			}
		}
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastStayput : DukeNewBeast
{
	default
	{
		pic "NEWBEASTSTAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AINEWBEASTGETENEMY');
		self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		self.ChangeType('DukeNewBeast');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastHang : DukeNewBeast
{
	default
	{
		pic "NEWBEASTHANG";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('ANEWBEASTHANG');
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.Scale = (40 * REPEAT_SCALE, 40 * REPEAT_SCALE);
		}
		else if (self.ifhitbyweapon() >= 0)
		{
			self.ChangeType('DukeNewBeast');
			setAction('ANEWBEASTSTAND');
			self.PlayActorSound("NEWBEAST_PAIN");
		}
		else
		{
			if (self.attackertype.GetClassName() == 'DukeBoss4')
			{
				if (self.counter >= 200)
				{
					if (Duke.rnd(1))
					{
						self.ChangeType('DukeNewBeast');
						setAction('ANEWBEASTSTAND');
						self.PlayActorSound("NEWBEAST_PAIN");
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

class DukeNewBeastHangDead : DukeNewBeast // (4671)
{
	default
	{
		pic "NEWBEASTHANGDEAD";
		-KILLCOUNT;
		Strength TOUGH;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('ANEWBEASTHANGDEAD');
			self.Scale = (40 * REPEAT_SCALE, 40 * REPEAT_SCALE);
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.extra < 0)
				{
					state_standard_jibs(p, pdist);
					if (self.sector != null) self.spawn('DukeBloodPool');
					self.PlayActorSound("SQUISHED");
					self.killit();
				}
				else
				{
					self.spawnguts('DukeJibs6', 1);
					self.PlayActorSound("SQUISHED");
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

class DukeNewBeastJump : DukeNewBeast // (4690)
{
	default
	{
		pic "NEWBEASTJUMP";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AINEWBEASTJUMPENEMY');
		self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		self.ChangeType('DukeNewBeast');
	}
	
}

