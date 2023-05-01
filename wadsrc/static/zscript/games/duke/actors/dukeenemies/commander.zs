class DukeCommander : DukeActor
{
	const COMMANDERSTRENGTH = 350;
	const CAPTSPINNINGPLAYER = -11;
	
	default
	{
		pic "COMMANDER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		+FLOATING;
		+SHOOTCENTERED;
		+NORANDOMANGLEWHENBLOCKED;
		+NOHITJIBS;
		gutsoffset -24;
		falladjustz 0;
		floating_floordist 8;
		floating_ceilingdist 80;
		Strength COMMANDERSTRENGTH;
		
		action "ACOMMBREETH", 0, 3, 5, 1, 40;
		action "ACOMMFROZEN", 0, 1, 5;
		action "ACOMMSPIN", -5, 1, 5, 1, 12;
		action "ACOMMGET", 0, 3, 5, 1, 30;
		action "ACOMMSHOOT", 20, 1, 5, 1, 35;
		action "ACOMMABOUTTOSHOOT", 20, 1, 5, 1, 30;
		action "ACOMMDYING", 30, 8, 1, 1, 12;
		action "ACOMMDEAD", 38, 1, 1, 1, 1;
		move "COMMGETUPVELS", 128, -64;
		move "COMMGETVELS", 128, 64;
		move "COMMSLOW", 64, 24;
		move "COMMSTOPPED";
		ai "AICOMMWAIT", "ACOMMBREETH", "COMMSTOPPED", faceplayerslow;
		ai "AICOMMGET", "ACOMMGET", "COMMGETVELS", seekplayer;
		ai "AICOMMSHOOT", "ACOMMSHOOT", "COMMSTOPPED", faceplayerslow;
		ai "AICOMMABOUTTOSHOOT", "ACOMMABOUTTOSHOOT", "COMMSTOPPED", faceplayerslow;
		ai "AICOMMSPIN", "ACOMMSPIN", "COMMGETVELS", spin;
		ai "AICOMMDYING", "ACOMMDYING", "COMMSTOPPED", faceplayer;
		ai "AICOMMSHRUNK", "ACOMMGET", "COMMSLOW", furthestdir;
		ai "AICOMMGROW", "ACOMMGET", "COMMSTOPPED", furthestdir;
		ai "AICOMMONFIRE", "ACOMMGET", "COMMGETVELS", fleeenemy;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("COMM_RECOG");
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_commonfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AICOMMGET');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("COMM_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AICOMMDYING');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkcommhitstate(DukePlayer p, double pdist)
	{
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.attackertype.GetClassName() != 'DukeFlamethrowerFlame')
			{
				self.spawnguts('DukeJibs6', 2);
			}
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.PlayActorSound("SOMETHINGFROZE");
					self.tempval = self.pal;
					self.pal = 1;
					setMove('none', 0);
					setAction('ACOMMFROZEN');
					self.extra = 0;
					return;
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeGrowSpark')
					{
						self.PlayActorSound("ACTOR_GROWING");
						setAI('AICOMMGROW');
						return;
					}
				}
				self.addkill();
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
					self.PlayActorSound("SQUISHED");
					state_standard_jibs(p, pdist);
					self.killit();
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeRPG')
					{
						self.PlayActorSound("SQUISHED");
						if (self.sector != null) self.spawn('DukeBloodPool');
						state_standard_jibs(p, pdist);
						self.killit();
					}
				}
				self.PlayActorSound("COMM_DYING");
				setAI('AICOMMDYING');
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
				{
				}
				else
				{
					self.PlayActorSound("COMM_PAIN", CHAN_AUTO, CHANF_SINGULAR);
				}
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					if (self.pal == 2)
					{
						self.pal = self.tempval;
						self.tempval = 0;
					}
					self.PlayActorSound("ACTOR_SHRINKING");
					setAI('AICOMMSHRUNK');
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
					else if (self.curAI != 'AICOMMSHRUNK')
					{
						self.tempval = self.pal;
						self.pal = 2;
						setAI('AICOMMONFIRE');
						self.counter = 0;
						state_commonfirestate(p, pdist);
					}
				}
				else if (self.curAI == 'AICOMMONFIRE')
				{
					return;
				}
				else if (Duke.rnd(24))
				{
					setAI('AICOMMABOUTTOSHOOT');
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
		state_checksquished(p, pdist);
		if (self.curAction.name == 'ACOMMFROZEN')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			if (self.counter >= THAWTIME)
			{
				self.pal = self.tempval;
				self.tempval = 0;
				setAI('AICOMMWAIT');
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
		if (self.curAI == 'none')
		{
			setAI('AICOMMSHOOT');
		}
		else if (self.curAI == 'AICOMMWAIT')
		{
			if (self.counter >= 20)
			{
				if (self.cansee(p))
				{
					if (self.ifcanshoottarget(p, pdist))
					{
						if (Duke.rnd(96))
						{
							setAI('AICOMMGET');
						}
						else
						{
							setAI('AICOMMABOUTTOSHOOT');
						}
					}
				}
				else
				{
					setAI('AICOMMGET');
				}
			}
		}
		else if (self.curAI == 'AICOMMABOUTTOSHOOT')
		{
			if (self.actioncounter >= 2)
			{
				if (self.cansee(p))
				{
					setAI('AICOMMSHOOT');
				}
				else
				{
					setAI('AICOMMGET');
					return;
				}
			}
			if (Duke.rnd(32))
			{
				self.PlayActorSound("COMM_ATTACK", CHAN_AUTO, CHANF_SINGULAR);
			}
		}
		else if (self.curAI == 'AICOMMSHOOT')
		{
			if (self.ifcanshoottarget(p, pdist))
			{
				if (self.counter >= 24)
				{
					if (Duke.rnd(16))
					{
						setAI('AICOMMWAIT');
					}
				}
				if (self.actioncounter >= 2)
				{
					self.shoot('DukeRPG');
					self.actioncounter = 0;
				}
			}
			else
			{
				setAI('AICOMMGET');
			}
		}
		else if (self.curAI == 'AICOMMSHRUNK')
		{
			if (self.counter >= SHRUNKDONECOUNT)
			{
				setAI('AICOMMGET');
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
		else if (self.curAI == 'AICOMMGROW')
		{
			state_genericgrowcode(p, pdist, 100 * REPEAT_SCALE, 100 * REPEAT_SCALE);
		}
		else if (self.curAI == 'AICOMMGET')
		{
			if (self.movflag > kHitSector)
			{
				if (Duke.rnd(4))
				{
					self.actoroperate();
				}
			}
			if (pdist < 1024 * maptoworld)
			{
				if (self.checkp(p, palive))
				{
					self.PlayActorSound("COMM_SPIN");
					setAI('AICOMMSPIN');
					return;
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			if (self.cansee(p))
			{
				if (self.checkp(p, phigher))
				{
					setMove('COMMGETUPVELS', getv | geth | faceplayer);
				}
				else
				{
					setMove('COMMGETVELS', getv | geth | faceplayer);
				}
			}
			if (self.actioncounter >= 8)
			{
				if (Duke.rnd(2))
				{
					setAI('AICOMMABOUTTOSHOOT');
				}
			}
		}
		else if (self.curAI == 'AICOMMSPIN')
		{
			self.PlayActorSound("COMM_SPIN", CHAN_AUTO, CHANF_SINGULAR);
			if (self.counter >= 16)
			{
				if (pdist < 1280 * maptoworld)
				{
					p.addphealth(CAPTSPINNINGPLAYER, false);
					self.PlayActorSound("PLAYER_GRUNT");
					p.pals = color(32, 16, 0, 0);
					self.counter = 0;
				}
				else
				{
					if (pdist > 2300 * maptoworld)
					{
						setAI('AICOMMWAIT');
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			if (self.actioncounter >= 52)
			{
				setAI('AICOMMWAIT');
			}
			if (self.movflag > kHitSector)
			{
				if (Duke.rnd(32))
				{
					self.actoroperate();
				}
			}
		}
		else if (self.curAI == 'AICOMMONFIRE')
		{
			state_commonfirestate(p, pdist);
		}
		if (self.curAI == 'AICOMMDYING')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			self.extra = 0;
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					self.PlayActorSound("SQUISHED");
					if (self.sector != null) self.spawn('DukeBloodPool');
					state_standard_jibs(p, pdist);
					self.killit();
				}
			}
			if (self.curAction.name == 'ACOMMDYING')
			{
				if (self.actioncounter >= 8)
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
					self.cstat = 0;
					setAction('ACOMMDEAD');
				}
			}
		}
		else
		{
			if (Duke.rnd(2))
			{
				self.PlayActorSound("COMM_ROAM", CHAN_AUTO, CHANF_SINGULAR);
			}
			state_checkcommhitstate(p, pdist);
		}
	}
	
	
}

class DukeCommanderStayput: DukeCommander
{
	default
	{
		pic "COMMANDERSTAYPUT";
		+BADGUYSTAYPUT;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.ChangeType('DukeCommander');
		setAI('AICOMMABOUTTOSHOOT');
	}
	
	
}
