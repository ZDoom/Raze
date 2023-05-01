

class DukeBoss4 : DukeBossBase
{
	const BOSS4STRENGTH = 6000;
	const BOSS4PALSTRENGTH = 1000;

	default
	{
		pic "BOSS4";
		-ALTHITSCANDIRECTION;
		Strength BOSS4STRENGTH;

		action "ABOSS4WALK", 0, 4, 5, 1, 30;
		action "ABOSS4DYING", 40, 9, 1, 1, 20;
		action "ABOSS4ABOUTTOSHOOT", 20, 1, 5, 1, 40;
		action "ABOSS4SHOOT", 25, 2, 5, 1, 10;
		action "ABOSS4LAYIT", 50, 3, 5, 1, 120;
		action "BOSS4FLINTCH", 40, 1, 1, 1, 1;
		action "ABOSS4DEAD", 49;
		move "BOSS4WALKVELS", 128;
		move "BOSS4STOPPED";
		ai "AIBOSS4LAYEGGS", "ABOSS4WALK", "BOSS4WALKVELS", randomangle| geth;
		ai "AIBOSS4SHOOT", "ABOSS4ABOUTTOSHOOT", "BOSS4STOPPED", faceplayer;
		ai "AIBOSS4DYING", "ABOSS4DYING", "BOSS4STOPPED", faceplayer;
		
	}
	
	override void PlayFTASound(int mode)
	{
		if (self.pal == 1)
			Duke.PlaySound("BOS4_RECOG");
		Duke.PlaySound("BOSS4_FIRSTSEE");
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss4shootstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS4ABOUTTOSHOOT')
		{
			if (self.actioncounter >= 3)
			{
				setAction('ABOSS4SHOOT');
			}
		}
		if (self.curAction.name == 'ABOSS4SHOOT')
		{
			if (self.counter >= 48)
			{
				if (Duke.rnd(4))
				{
					setAI('AIBOSS4LAYEGGS');
				}
			}
			if (self.counter >= 26)
			{
				if (Duke.rnd(32))
				{
					if (Duke.rnd(128))
					{
						self.PlayActorSound("SHORT_CIRCUIT");
						p.addphealth(-2, self.bBIGHEALTH);
					}
					else
					{
						self.PlayActorSound("PLAYER_GRUNT");
						p.addphealth(-1, self.bBIGHEALTH);
					}
					p.pals = color(32, 32, 0, 0);
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss4layeggs(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			self.PlayActorSound("BOS4_ROAM");
		}
		if (self.curAction.name == 'ABOSS4LAYIT')
		{
			if (self.actioncounter >= 3)
			{
				if (self.counter >= 32)
				{
					setAI('AIBOSS4LAYEGGS');
					if (Duke.rnd(32))
					{
						setMove('BOSS4WALKVELS', furthestdir | geth);
					}
					if (self.sector != null) self.spawn('DukeNewBeastHang');
				}
			}
		}
		else
		{
			if (self.counter >= 64)
			{
				if (Duke.rnd(4))
				{
					setMove('none', 0);
					if (Duke.rnd(88))
					{
						setAction('ABOSS4LAYIT');
						self.PlayActorSound("BOS4_LAY");
					}
					else
					{
						if (self.checkp(p, palive))
						{
							if (self.cansee(p))
							{
								setAI('AIBOSS4SHOOT');
								self.PlayActorSound("BOS4_ATTACK");
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

	void state_boss4dyingstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ABOSS4DEAD')
		{
			return;
		}
		else
		{
			if (self.actioncounter >= 9)
			{
				if (self.floorz - self.pos.Z < 8)
				{
					self.PlayActorSound("THUD");
				}
				if (self.pal == 0 || !isWorldTour())
				{
					p.timebeforeexit = 52;
					p.customexitsound = -1;
					ud.eog = true;
				}
				setAction('ABOSS4DEAD');
				self.cstat = 0;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkboss4hitstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(2))
		{
			if (self.sector != null) self.spawn('DukeBloodPool');
		}
		if (self.extra < 0)
		{
			p.actor.PlayActorSound("PLAYER_TALKTOBOSSFALL", CHAN_AUTO, CHANF_LOCAL);
			self.addkill();
			setAI('AIBOSS4DYING');
			self.PlayActorSound("BOS4_DYING");
			self.PlayActorSound("BOSS4_DEADSPEECH");
		}
		else
		{
			self.PlayActorSound("BOS4_PAIN", CHAN_AUTO, CHANF_SINGULAR);
			self.spawndebris(DukeScrap.Scrap1, 1);
			self.spawnguts('DukeJibs6', 1);
			if (self.curAction.name == 'ABOSS4LAYIT')
			{
				return;
			}
			if (Duke.rnd(16))
			{
				setAction('BOSS4FLINTCH');
				setMove('none', 0);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_boss4code(DukePlayer p, double pdist)
	{
		if (self.curAI == 'none')
		{
			if (self.pal == 21 || !isWorldTour())
			{
				self.extra = BOSS4PALSTRENGTH;
			}
			setAI('AIBOSS4LAYEGGS');
		}
		else if (self.curAction.name == 'BOSS4FLINTCH')
		{
			if (self.actioncounter >= 3)
			{
				setAI('AIBOSS4LAYEGGS');
			}
		}
		else if (self.curAI == 'AIBOSS4LAYEGGS')
		{
			state_boss4layeggs(p, pdist);
		}
		else if (self.curAI == 'AIBOSS4SHOOT')
		{
			state_boss4shootstate(p, pdist);
		}
		if (self.curAI == 'AIBOSS4DYING')
		{
			state_boss4dyingstate(p, pdist);
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkboss4hitstate(p, pdist);
			}
			else
			{
				if (self.checkp(p, palive))
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

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.ChangeType('DukeBoss4');
		state_boss4code(p, pdist);
	}

	
}


class DukeBoss4Stayput : DukeBoss4
{
	default
	{
		pic "BOSS4STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.ChangeType('DukeBoss4');
		state_boss4code(p, pdist);
	}
	
}
	
