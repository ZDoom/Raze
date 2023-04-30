
class DukeLizMan : DukeActor
{
	const LIZSTRENGTH = 100;
	default
	{
		pic "LIZMAN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+DONTENTERWATER;
		+RANDOMANGLEONWATER;
		precacheclass "DukeLizmanHead", "DukeLizmanArm", "DukeLizmanLeg";
		Strength LIZSTRENGTH;

		action "ALIZSTAND", 0;
		action "ALIZWALKING", 0, 4, 5, 1, 15;
		action "ALIZRUNNING", 0, 4, 5, 1, 11;
		action "ALIZTHINK", 20, 2, 5, 1, 40;
		action "ALIZSCREAM", 30, 1, 5, 1, 2;
		action "ALIZJUMP", 45, 3, 5, 1, 20;
		action "ALIZFALL", 55, 1, 5;
		action "ALIZSHOOTING", 70, 2, 5, 1, 7;
		action "ALIZDYING", 60, 6, 1, 1, 15;
		action "ALIZLYINGDEAD", 65, 1;
		action "ALIZFROZEN", 0, 1, 5;
		move "LIZWALKVEL", 72;
		move "LIZRUNVEL", 192;
		move "LIZJUMPVEL", 184;
		move "LIZSTOP";
		ai "AILIZGETENEMY", "ALIZWALKING", "LIZWALKVEL", seekplayer;
		ai "AILIZDODGE", "ALIZRUNNING", "LIZRUNVEL", dodgebullet;
		ai "AILIZCHARGEENEMY", "ALIZRUNNING", "LIZRUNVEL", seekplayer;
		ai "AILIZFLEENEMY", "ALIZWALKING", "LIZWALKVEL", fleeenemy;
		ai "AILIZSHOOTENEMY", "ALIZSHOOTING", "LIZSTOP", faceplayer;
		ai "AILIZJUMPENEMY", "ALIZJUMP", "LIZJUMPVEL", jumptoplayer;
		ai "AILIZTHINK", "ALIZTHINK", "LIZSTOP", faceplayerslow;
		ai "AILIZSHRUNK", "ALIZWALKING", "SHRUNKVELS", fleeenemy;
		ai "AILIZGROW", "ALIZSTAND", "LIZSTOP", faceplayerslow;
		ai "AILIZSPIT", "ALIZSCREAM", "LIZSTOP", faceplayerslow;
		ai "AILIZDYING", "ALIZDYING", "LIZSTOP", faceplayer;
		ai "AILIZONFIRE", "ALIZWALKING", "LIZWALKVEL", fleeenemy;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("CAPT_RECOG");
	}
	
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_liz_body_jibs(DukePlayer p, double pdist)
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

	void state_lizonfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AILIZGETENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("CAPT_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AILIZDYING');
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizseekstate(DukePlayer p, double pdist)
	{
		if (self.actorstayput == null)
		{
			if (self.cansee(p))
			{
				if (self.checkp(p, palive))
				{
					if (pdist < 2048 * maptoworld)
					{
						if (self.counter >= 16)
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								setAI('AILIZSHOOTENEMY');
								return;
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			if (self.curAI == 'AILIZCHARGEENEMY')
			{
				if (self.counter >= 72)
				{
					if (self.ifcanshoottarget(p, pdist))
					{
						setAI('AILIZSHOOTENEMY');
						return;
					}
				}
				if (self.checkp(p, phigher))
				{
					if (pdist > 2048 * maptoworld)
					{
						if (Duke.rnd(6))
						{
							setAI('AILIZJUMPENEMY');
							return;
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			else
			{
				if (pdist > 4096 * maptoworld)
				{
					if (Duke.rnd(92))
					{
						if (self.counter >= 48)
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								setAI('AILIZSHOOTENEMY');
							}
						}
					}
					else
					{
						if (self.counter >= 24)
						{
							setAI('AILIZCHARGEENEMY');
							return;
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			if (self.floorz - self.pos.Z < 16)
			{
				if (self.counter >= 48)
				{
					if (self.movflag > kHitSector)
					{
						if (self.cansee(p))
						{
							setAI('AILIZJUMPENEMY');
							return;
						}
					}
				}
			}
			else
			{
				if (pdist > 1280 * maptoworld)
				{
					setAI('AILIZJUMPENEMY');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
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
				if (Duke.rnd(1))
				{
					if (self.dodge() == 1)
					{
						if (self.floorz - self.ceilingz < 128)
						{
							setAI('AILIZDODGE');
						}
						else
						{
							if (self.actorstayput == null)
							{
								if (Duke.rnd(32))
								{
									setAI('AILIZJUMPENEMY');
								}
								else
								{
									setAI('AILIZDODGE');
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
					if (Duke.rnd(32))
					{
						if (self.cansee(p))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								setAI('AILIZSHOOTENEMY');
							}
						}
					}
				}
			}
			if (self.counter >= 16)
			{
				if (Duke.rnd(32))
				{
					setMove('LIZWALKVEL', randomangle | geth);
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizshrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI('AILIZGETENEMY');
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

	void state_lizfleestate(DukePlayer p, double pdist)
	{
		if (self.counter >= 16)
		{
			if (Duke.rnd(48))
			{
				if (self.checkp(p, palive))
				{
					if (self.cansee(p))
					{
						setAI('AILIZSPIT');
					}
				}
			}
		}
		else
		{
			if (self.floorz - self.pos.Z < 16)
			{
			}
			else
			{
				setAI('AILIZGETENEMY');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizthinkstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(8))
		{
			self.PlayActorSound("CAPT_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
		if (self.actioncounter >= 3)
		{
			if (Duke.rnd(32))
			{
				if (self.checkp(p, palive))
				{
					if (self.cansee(p))
					{
						setAI('AILIZSPIT');
					}
				}
			}
			else
			{
				if (Duke.rnd(96))
				{
					setAI('AILIZGETENEMY');
				}
			}
		}
		else
		{
			if (self.actioncounter >= 2)
			{
				if (Duke.rnd(1))
				{
					if (self.sector != null) self.spawn('DukeFeces');
				}
			}
		}
		if (Duke.rnd(1))
		{
			if (self.dodge() == 1)
			{
				if (self.floorz - self.ceilingz < 96)
				{
					setAI('AILIZDODGE');
				}
				else
				{
					if (Duke.rnd(128))
					{
						setAI('AILIZJUMPENEMY');
					}
					else
					{
						setAI('AILIZDODGE');
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

	void state_lizshootstate(DukePlayer p, double pdist)
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
							setAI('AILIZFLEENEMY');
						}
						return;
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				if (Duke.rnd(80))
				{
					setAI('AILIZTHINK');
				}
				else
				{
					setAI('AILIZGETENEMY');
				}
			}
		}
		if (self.actioncounter >= 2)
		{
			if (self.cansee(p))
			{
				if (self.ifcanshoottarget(p, pdist))
				{
					self.PlayActorSound("CAPT_ATTACK");
					self.shoot('DukeShotSpark');
					self.actioncounter = 0;
				}
				else
				{
					setAI('AILIZTHINK');
				}
			}
			else
			{
				setAI('AILIZGETENEMY');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checklizhit(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.curAI == 'AILIZSHRUNK')
		{
			self.addkill();
			self.PlayActorSound("SQUISHED");
			state_standard_jibs(p, pdist);
			self.killit();
		}
		if (self.extra < 0)
		{
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				setMove('none', 0);
				setAction('ALIZFROZEN');
				self.extra = 0;
				return;
			}
			state_drop_chaingun(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.cstat = 0;
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AILIZGROW');
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.PlayActorSound("SQUISHED");
				state_liz_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.PlayActorSound("SQUISHED");
				state_liz_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				state_spawnburnedcorpse(p, pdist);
			}
			else
			{
				state_rf(p, pdist);
				setAI('AILIZDYING');
				if (Duke.rnd(64))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
			}
			self.PlayActorSound("CAPT_DYING");
		}
		else
		{
			if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
			}
			else
			{
				self.PlayActorSound("CAPT_PAIN");
			}
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				if (self.pal == 2)
				{
					self.pal = self.tempval;
					self.tempval = 0;
				}
				self.PlayActorSound("ACTOR_SHRINKING");
				setAI('AILIZSHRUNK');
				return;
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			state_random_wall_jibs(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				if (self.pal == 2)
				{
				}
				else
				{
					self.tempval = self.pal;
					self.pal = 2;
					setAI('AILIZONFIRE');
					self.counter = 0;
					state_lizonfirestate(p, pdist);
				}
			}
			else if (self.curAI != 'AILIZONFIRE')
			{
				if (self.checkp(p, palive))
				{
					if (self.cansee(p))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							setAI('AILIZSHOOTENEMY');
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

	void state_lizjumpstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ALIZFALL')
		{
			if (self.floorz - self.pos.Z < 16)
			{
				setAI('AILIZGETENEMY');
			}
		}
		else
		{
			if (self.actioncounter >= 3)
			{
				setAction('ALIZFALL');
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizdyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ALIZLYINGDEAD')
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
				if (ud.respawn_monsters)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = LIZSTRENGTH;
					setAI('AILIZGETENEMY');
				}
			}
		}
		else
		{
			if (self.curAI == 'AILIZDYING')
			{
				if (self.actioncounter >= 6)
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
					setMove('LIZSTOP', 0);
					setAction('ALIZLYINGDEAD');
				}
			}
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizdodgestate(DukePlayer p, double pdist)
	{
		if (self.counter >= 13)
		{
			setAI('AILIZGETENEMY');
		}
	}
	
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_lizcode(DukePlayer p, double pdist)
	{
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			setAI('AILIZGETENEMY');
		}
		else if (self.curAction.name == 'ALIZLYINGDEAD')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			state_lizdyingstate(p, pdist);
			return;
		}
		else if (self.curAction.name == 'ALIZFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AILIZGETENEMY');
				self.pal = self.tempval;
				self.tempval = 0;
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
		else if (self.curAI == 'AILIZJUMPENEMY')
		{
			state_lizjumpstate(p, pdist);
		}
		else
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			if (self.curAI == 'AILIZGETENEMY')
			{
				state_lizseekstate(p, pdist);
			}
			else if (self.curAI == 'AILIZCHARGEENEMY')
			{
				state_lizseekstate(p, pdist);
			}
			else if (self.curAI == 'AILIZDODGE')
			{
				state_lizdodgestate(p, pdist);
			}
			else if (self.curAI == 'AILIZSHOOTENEMY')
			{
				state_lizshootstate(p, pdist);
			}
			else if (self.curAI == 'AILIZFLEENEMY')
			{
				state_lizfleestate(p, pdist);
			}
			else if (self.curAI == 'AILIZTHINK')
			{
				state_lizthinkstate(p, pdist);
			}
			else if (self.curAI == 'AILIZSHRUNK')
			{
				state_lizshrunkstate(p, pdist);
			}
			else if (self.curAI == 'AILIZGROW')
			{
				state_genericgrowcode(p, pdist);
			}
			else if (self.curAI == 'AILIZDYING')
			{
				state_lizdyingstate(p, pdist);
			}
			else if (self.curAI == 'AILIZONFIRE')
			{
				state_lizonfirestate(p, pdist);
			}
			else
			{
				if (self.curAI == 'AILIZSPIT')
				{
					if (self.counter >= 26)
					{
						setAI('AILIZGETENEMY');
					}
					else
					{
						if (self.counter >= 18)
						{
							if (Duke.rnd(96))
							{
								self.shoot('DukeSpit');
								self.PlayActorSound("LIZARD_SPIT");
							}
						}
					}
				}
			}
		}
		if (self.curAI == 'AILIZSHRUNK')
		{
			return;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_checklizhit(p, pdist);
		}
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		state_lizcode(p, pdist);
	}
}

	
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizManSpitting : DukeLizMan
{
	default
	{
		pic "LIZMANSPITTING";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AILIZSPIT');
		self.ChangeType('DukeLizMan');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizManFeeding : DukeLizMan
{
	// this one has setup code but no implementation.
	default
	{
		pic "LIZMANFEEDING";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizManJump : DukeLizMan
{
	default
	{
		pic "LIZMANJUMP";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AILIZJUMPENEMY');
		self.ChangeType('DukeLizMan');
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizManStayput : DukeLizMan
{
	default
	{
		pic "LIZMANSTAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void PlayFTASound(int mode)
	{
	}

	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AILIZGETENEMY');
		self.ChangeType('DukeLizMan');
	}

}

