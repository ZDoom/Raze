class NWinterCommander : DukeCommander
{
	default
	{
		action "ACOMMSPIN", 0, 1, 5, 1, 12;
		ai "AICOMMSPIN", "ACOMMSPIN", "COMMGETVELS", spin;
	}
	
	void state_checknwcommhitstate(DukePlayer p, double pdist)
	{
		if (self.ifhitbyweapon() >= 0)
		{
			self.spawndebris(DukeScrap.Scrap3, 5);
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					return;
				}
				else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.PlayActorSound("ACTOR_GROWING");
					setAI('AICOMMGROW');
					return;
				}
				self.addkill();
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					self.spawndebris(DukeScrap.Scrap3, 5);
					self.PlayActorSound("SQUISHED");
					self.killit();
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeRPG')
					{
						self.PlayActorSound("SQUISHED");
						self.spawndebris(DukeScrap.Scrap3, 5);
						state_standard_jibs(p, pdist);
						self.killit();
					}
				}
				self.spawndebris(DukeScrap.Scrap3, 100);
				self.PlayActorSound("COMM_DYING");
				setAI('AICOMMDYING');
			}
			else
			{
				self.PlayActorSound("COMM_PAIN", CHAN_AUTO, CHANF_SINGULAR);
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					self.PlayActorSound("ACTOR_SHRINKING");
					setAI('AICOMMSHRUNK');
				}
				else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.PlayActorSound("EXPANDERHIT");
				}
				else
				{
					if (Duke.rnd(24))
					{
						setAI('AICOMMABOUTTOSHOOT');
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
					self.shoot('DukeFreezeBlast');
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
			state_genericgrowcode(p, pdist);
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
					if (self.floorz - self.pos.Z < 8)
					{
						self.PlayActorSound("THUD");
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
			state_checknwcommhitstate(p, pdist);
		}
	}
	
	
}

class NWinterCommanderStayput: NWinterCommander
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
