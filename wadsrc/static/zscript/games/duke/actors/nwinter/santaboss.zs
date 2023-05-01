class NWinterSanta : DukeActor
{
	const SOBBOTSTRENGTH = 2500;
	const MINIBOSSSTRENGTH = 100;

	default
	{
		+BADGUY;
		+KILLCOUNT;
		pic "PIGCOP";	// tiles are offset from here.
		move "botshrunkspeed", 50;
		move "botsteroidspeed", 400;
		move "botstopped";
		move "botfall", 0, -38;
		move "bothardfall", 0;
		move "botflyupspeed", 200, -140;
		move "botflydownspeed", 200, -38;
		move "botflyspeed", 200;
		move "botdodgespeedfly", 250, -140;
		move "botdodgespeeddownfly", 250, 140;
		action "ABOTFLY", 1860, 1, 5, 1, 13;
		action "ABOTFLYDYING", 1925, 5, 1, 1, 18;
		action "ABOTFLYDIEBACK", 1929, 5, 1, -1, 18;
		action "ABOTFLYDEAD", 1925, 1, 1, 1, 1;
		action "ABOTFLYFROZEN", 1860, 1, 5, 1, 1;
		ai "AIBOTFLYUP", "ABOTFLY", "botflyupspeed", faceplayer;
		ai "AIBOTFLYDOWN", "ABOTFLY", "botflydownspeed", faceplayer;
		ai "AIBOTFLYSEEKUP", "ABOTFLY", "botflyupspeed", geth| seekplayer| randomangle;
		ai "AIBOTFLYSEEKDOWN", "ABOTFLY", "botflydownspeed", geth| seekplayer| randomangle;
		ai "AIBOTFLYDODGEUP", "ABOTFLY", "botdodgespeedfly", fleeenemy| randomangle;
		ai "AIBOTFLYDODGEDOWN", "ABOTFLY", "botdodgespeeddownfly", fleeenemy| randomangle;
		ai "AIBOTFLYNOTMOVING", "none", "none";
		ai "AIBOTFLYCAMPING", "ABOTFLY", "botstopped", faceplayer;
		ai "AIBOTFLYDYING", "ABOTFLYDYING", "botstopped", faceplayer;
		ai "AIFLYWAIT", "ABOTFLY", "botstopped", faceplayer;
		ai "AIFLYUPMOMENT", "ABOTFLY", "botflyupspeed", faceplayer;
		ai "AIFLYUPHOLD", "ABOTFLY", "botflyupspeed", faceplayer;
		ai "AIBOTFLYSTRAIGHT", "ABOTFLY", "botflyspeed", geth| getv;
		ai "AIBOTFLYGROW", "ABOTFLY", "botstopped", geth| getv;
		ai "AIBOTFLYSHRINK", "ABOTFLY", "botshrunkspeed", fleeenemy;
		ai "AIBOTFLYSTEROIDS", "ABOTFLY", "botsteroidspeed", randomangle;
		ai "AIBOTFLYFLEE", "ABOTFLY", "botflyspeed", randomangle;
		action "ABOTWALK", 1905, 4, 5, 1, 13;
		action "ABOTWALK1", 1905, 1, 5, 1, 13;
		action "ABOTWALKGO", 1905, 4, 5, 1, 13;
		action "ABOTKICK", 1905, 2, 5, 1, 25;
		action "ABOTCRAWL", -2096, 3, 5, 1, 13;
		action "ABOTJUMPUP", 1940, 2, 5, 1, 13;
		action "ABOTFALL", 1950, 1, 5, 1, 1;
		action "ABOTWDYING", 1970, 5, 1, 1, 18;
		action "ABOTWDEAD", 1975, 1, 1, 1, 1;
		action "ABOTWALKDIEBACK", 1974, 5, 1, -1, 18;
		action "ABOTWALKFROZEN", 1905, 1, 5, 1, 1;
		move "botwalkspeed", 250;
		move "botjumpspeed", 250, -115;
		move "botjumpdspeed", 250, 150;
		move "botcrawlspeed", 0;
		move "botfallspeed", 250, 100;
		ai "AIBOTWALKKICK", "ABOTKICK", "botstopped", faceplayer;
		ai "AIBOTWALKHUNT", "ABOTWALK", "botwalkspeed", faceplayer;
		ai "AIBOTWALKSEEK", "ABOTWALK", "botwalkspeed", seekplayer;
		ai "AIBOTWALKSEEK2", "none", "none";
		ai "AIBOTWALKCAMPING", "ABOTWALK", "botstopped", faceplayersmart;
		ai "AIBOTWALKSTRJUMP1", "ABOTJUMPUP", "botjumpspeed", geth| getv;
		ai "AIBOTWALKSTRJUMP2", "ABOTFALL", "botjumpdspeed", geth| getv;
		ai "AIBOTWALKSTRJUMP1S", "ABOTJUMPUP", "botjumpspeed", faceplayer;
		ai "AIBOTWALKSTRJUMP2S", "ABOTFALL", "botjumpdspeed", faceplayer;
		ai "AIBOTWALKJUMP1", "ABOTJUMPUP", "botjumpspeed", faceplayer;
		ai "AIBOTWALKJUMP2", "ABOTFALL", "botjumpdspeed", faceplayer;
		ai "AIBOTWALKDYING", "ABOTWDYING", "botstopped", faceplayer;
		ai "AIWALKFALL", "ABOTFALL", "botstopped", faceplayer;
		ai "AIWALKFALLHARD", "ABOTFALL", "botstopped", faceplayer;
		ai "AIBOTWALKSTRAIGHT", "ABOTWALK", "botwalkspeed", geth| getv;
		ai "AIBOTWALKGROW", "ABOTWALK1", "botstopped", geth| getv;
		ai "AIBOTWALKSHRINK", "ABOTWALK", "botshrunkspeed", fleeenemy;
		ai "AIBOTWALKSTEROIDS", "ABOTWALK", "botsteroidspeed", randomangle;
		ai "AIBOTWALKFLEE", "ABOTWALK", "botwalkspeed", randomangle;
		ai "AIBOTLETFALL", "ABOTFALL", "botfallspeed", getv| geth;
		ai "AIBOTWALKNOTMOVING", "none", "none";
		ai "AIWALKWAIT", "ABOTWALK", "botstopped", faceplayer;
		Strength SOBBOTSTRENGTH;
		StartAction "ABOTWALK";
	}
	
	void state_miniboss(DukePlayer p, double pdist)
	{
		self.PlayActorSound("SANTA_TRASH1");
		self.spawn('DukeExplosion2');
		self.spawn('DukeExplosion2');
		self.spawn('NWinterSanta');
		self.spawn('NWinterSanta');
		self.killit();
	}
	void state_stomphisfreakingass(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pdead))
		{
		}
		else
		{
			if (pdist < SQUISHABLEDISTANCE * maptoworld)
			{
				p.addphealth(-1000, self.bBIGHEALTH);
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.PlayActorSound("SANTA_TRASH9");
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
	}
	void state_botchangeweaponstate(DukePlayer p, double pdist)
	{
		if (self.pal == 21)
		{
			return;
		}
		self.counter = 0;
		if (self.pal == 0)
		{
			if (pdist < 4096 * maptoworld)
			{
				if (Duke.rnd(128))
				{
					self.tempval = self.pal;
					self.pal = 3;
				}
				else
				{
					self.tempval = self.pal;
					self.pal = 9;
				}
			}
			else
			{
				if (Duke.rnd(128))
				{
					self.tempval = self.pal;
					self.pal = 3;
				}
			}
		}
		else if (self.pal == 3)
		{
			if (pdist > 4096 * maptoworld)
			{
				if (Duke.rnd(128))
				{
					self.tempval = self.pal;
					self.pal = 0;
				}
				else
				{
					self.tempval = self.pal;
					self.pal = 9;
				}
			}
			else
			{
				if (Duke.rnd(128))
				{
					self.tempval = self.pal;
					self.pal = 9;
				}
			}
		}
		else
		{
			if (self.pal == 9)
			{
				if (pdist > 4096 * maptoworld)
				{
					if (Duke.rnd(128))
					{
						self.tempval = self.pal;
						self.pal = 0;
					}
					else
					{
						self.tempval = self.pal;
						self.pal = 3;
					}
				}
				else
				{
					if (Duke.rnd(128))
					{
						self.tempval = self.pal;
						self.pal = 9;
					}
				}
			}
		}
		self.PlayActorSound("SELECT_WEAPON");
	}
	void state_botquickshootstate(DukePlayer p, double pdist)
	{
		if (self.pal == 21)
		{
			return;
		}
		if (self.checkp(p, pshrunk))
		{
			return;
		}
		if (self.checkp(p, pdead))
		{
			return;
		}
		if (self.pal == 0)
		{
			self.shoot('DukeRPG');
			self.PlayActorSound("RPG_SHOOT");
		}
		else if (self.pal == 3)
		{
			self.shoot('DukeChaingunShot');
			self.PlayActorSound("CHAINGUN_FIRE");
		}
		else
		{
			if (self.pal == 9)
			{
				self.shoot('DukeShotgunShot');
				self.PlayActorSound("SHOTGUN_FIRE");
			}
		}
	}
	void state_botshootstate(DukePlayer p, double pdist)
	{
		if (self.pal == 21)
		{
			return;
		}
		if (self.checkp(p, pdead))
		{
			return;
		}
		if (self.checkp(p, pshrunk))
		{
			return;
		}
		if (self.pal == 0)
		{
			if (self.counter >= 5)
			{
				if (self.actioncounter >= 5)
				{
					self.actioncounter = 0;
					self.shoot('DukeRPG');
					self.PlayActorSound("RPG_SHOOT");
				}
				if (pdist < 4096 * maptoworld)
				{
					if (Duke.rnd(5))
					{
						state_botchangeweaponstate(p, pdist);
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		else if (self.pal == 3)
		{
			if (self.counter >= 5)
			{
				if (self.actioncounter >= 1)
				{
					self.actioncounter = 0;
					self.shoot('DukeShotSpark');
					self.shoot('DukeChaingunShot');
					self.shoot('DukeChaingunShot');
					self.shoot('DukeChaingunShot');
					self.PlayActorSound("CHAINGUN_FIRE");
				}
				if (Duke.rnd(1))
				{
					state_botchangeweaponstate(p, pdist);
				}
			}
		}
		else
		{
			if (self.pal == 9)
			{
				if (self.counter >= 5)
				{
					if (self.actioncounter >= 5)
					{
						self.actioncounter = 0;
						self.shoot('DukeShotgunShot');
						self.PlayActorSound("SHOTGUN_FIRE");
					}
					if (self.checkp(p, ponsteroids))
					{
						if (Duke.rnd(64))
						{
							state_botchangeweaponstate(p, pdist);
						}
					}
					if (Duke.rnd(1))
					{
						state_botchangeweaponstate(p, pdist);
					}
				}
			}
		}
	}
	void state_dudehurt(DukePlayer p, double pdist)
	{
		if (Duke.rnd(80))
		{
			self.PlayActorSound("SANTA_GOTHIT");
		}
	}
	void state_dukehurt(DukePlayer p, double pdist)
	{
		if (Duke.rnd(80))
		{
			self.PlayActorSound("SANTA_GOTHIT");
		}
	}
	void state_botwalkhitstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.extra < 0)
		{
			if (self.pal == 21)
			{
				state_miniboss(p, pdist);
			}
			else
			{
				self.PlayActorSound("SANTA_DEAD", CHAN_AUTO, CHANF_SINGULAR);
			}
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.tempval = self.pal;
				self.pal = 1;
				self.extra = 0;
				setMove('none', 0);
				setAction('ABOTWALKFROZEN');
				self.PlayActorSound("SOMETHINGFROZE");
				return;
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.cstat = 0;
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AIBOTWALKGROW');
				return;
			}
			else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.PlayActorSound("SQUISHED");
				self.addkill();
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.PlayActorSound("SQUISHED");
				self.addkill();
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else
			{
				setAI('AIBOTWALKDYING');
				return;
			}
		}
		else
		{
			state_dudehurt(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				if (self.curAI == 'AIBOTWALKSTEROIDS')
				{
				}
				else
				{
					setAI('AIBOTWALKSHRINK');
					return;
				}
			}
		}
	}
	void state_botwalkdyingstate(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.extra = 0;
		if (self.actioncounter >= 5)
		{
			setAction('ABOTWDEAD');
			p.timebeforeexit = 52;
			p.customexitsound = -1;
			ud.eog = true;
			self.killit();
		}
	}
	void state_botwalkseekstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			setAI('AIBOTWALKHUNT');
			return;
		}
		else
		{
		}
		if (self.floorz - self.ceilingz < 100)
		{
		}
		else
		{
			if (self.movflag > kHitSector)
			{
				if (Duke.rnd(4))
				{
					self.actoroperate();
				}
				if (pdist < 8000 * maptoworld)
				{
					setAI('AIFLYUPMOMENT');
					self.ChangeType('NWinterSantaFly');
				}
				else
				{
					setAI('AIBOTWALKJUMP1');
				}
			}
		}
	}
	void state_botwalkjumpstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(4))
		{
			if (pdist < 6000 * maptoworld)
			{
				state_botquickshootstate(p, pdist);
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		if (self.curAI == 'AIBOTWALKJUMP1')
		{
			if (self.actioncounter >= 2)
			{
				setAI('AIBOTWALKJUMP2');
				return;
			}
		}
		if (self.curAI == 'AIBOTWALKJUMP2')
		{
			if (self.actioncounter >= 1)
			{
				state_botshootstate(p, pdist);
				if (self.floorz - self.pos.Z < 16)
				{
					self.PlayActorSound("PLAYER_LAND");
					self.actoroperate();
					setAI('AIBOTWALKHUNT');
				}
			}
		}
	}
	void state_botwalkhuntstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (Duke.rnd(3))
		{
			setAI('AIBOTWALKSTRJUMP1S');
			self.counter = 10;
		}
		if (self.cansee(p))
		{
			if (pdist < 1024 * maptoworld)
			{
				if (Duke.rnd(24))
				{
					if (self.checkp(p, pdead))
					{
					}
					else
					{
						setAI('AIBOTWALKKICK');
					}
					return;
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		else
		{
			if (p.actor.extra < 50)
			{
				if (Duke.rnd(64))
				{
					if (pdist < 8000 * maptoworld)
					{
						if (Duke.rnd(80))
						{
							self.PlayActorSound("SANTA_TRASH1");
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			if (Duke.rnd(128))
			{
				if (self.checkp(p, phigher))
				{
					setAI('AIFLYUPMOMENT');
					self.ChangeType('NWinterSantaFly');
					return;
				}
				else
				{
					setAI('AIBOTWALKSTRAIGHT');
					return;
				}
			}
			else
			{
				setAI('AIBOTWALKSTRAIGHT');
				return;
			}
			setAI('AIBOTWALKSEEK');
			return;
		}
		if (Duke.rnd(128))
		{
			if (self.movflag > kHitSector)
			{
				if (Duke.rnd(8))
				{
					self.actoroperate();
				}
				setAI('AIBOTWALKJUMP1');
				return;
			}
		}
	}
	void state_botwalkkickstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 2)
		{
			self.actioncounter = 0;
			if (pdist < 1024 * maptoworld)
			{
				if (self.checkp(p, pdead))
				{
					setAI('AIBOTWALKHUNT');
					return;
				}
				if (self.checkp(p, pfacing))
				{
					if (Duke.rnd(16))
					{
						p.actor.PlayActorSound("SANTA_TRASH8", CHAN_AUTO, CHANF_LOCAL);
					}
				}
				self.PlayActorSound("KICK_HIT");
				if (self.pal == 21)
				{
					p.addphealth(-1, self.bBIGHEALTH);
				}
				else
				{
					p.addphealth(-6, self.bBIGHEALTH);
				}
				state_dukehurt(p, pdist);
			}
			else
			{
				setAI('AIBOTWALKHUNT');
				return;
			}
			if (Duke.rnd(50))
			{
				setAI('AIBOTWALKHUNT');
				return;
			}
		}
	}
	void state_botwalkstraightjumpstate(DukePlayer p, double pdist)
	{
		if (self.curAI == 'AIBOTWALKSTRJUMP1')
		{
			if (self.counter >= 20)
			{
				setAI('AIBOTWALKSTRJUMP2');
				return;
			}
		}
		else
		{
			if (self.curAI == 'AIBOTWALKSTRJUMP2')
			{
				if (self.counter >= 18)
				{
					if (self.floorz - self.pos.Z < 16)
					{
						self.actoroperate();
						if (Duke.rnd(128))
						{
							self.PlayActorSound("PLAYER_LAND");
						}
						if (Duke.rnd(200))
						{
							setAI('AIBOTWALKSTRAIGHT');
						}
						else
						{
							setAI('AIBOTWALKSEEK');
						}
					}
				}
			}
		}
	}
	void state_botjumpshootstate(DukePlayer p, double pdist)
	{
		if (self.curAI == 'AIBOTWALKSTRJUMP1S')
		{
			state_botshootstate(p, pdist);
			if (self.counter >= 20)
			{
				setAI('AIBOTWALKSTRJUMP2S');
				self.actioncounter = 0;
				self.counter = 10;
			}
		}
		else
		{
			if (self.curAI == 'AIBOTWALKSTRJUMP2S')
			{
				if (self.counter >= 18)
				{
					if (self.floorz - self.pos.Z < 16)
					{
						self.actoroperate();
						self.PlayActorSound("PLAYER_LAND");
						setAI('AIBOTWALKHUNT');
						self.counter = 10;
					}
				}
			}
		}
	}
	void state_botwalkstraightstate(DukePlayer p, double pdist)
	{
		if (pdist > 20000 * maptoworld)
		{
			setAI('AIBOTWALKSEEK');
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.cansee(p))
		{
			if (self.movflag > kHitSector)
			{
				setAI('AIBOTWALKSTRJUMP1');
				return;
			}
			if (Duke.rnd(64))
			{
				setAI('AIBOTWALKHUNT');
			}
		}
		else
		{
			if (Duke.rnd(128))
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIBOTWALKSTRJUMP1');
				}
			}
		}
	}
	void state_steroidswalk(DukePlayer p, double pdist)
	{
		self.PlayActorSound("PLAYER_HARTBEAT", CHAN_AUTO, CHANF_SINGULAR);
		if (self.sector != null) self.spawn('DukeFrameEffect');
		if (self.ifhitbyweapon() >= 0)
		{
			state_botwalkhitstate(p, pdist);
			return;
		}
		if (self.counter >= 150)
		{
			setAI('AIBOTWALKHUNT');
		}
		if (self.counter >= 34)
		{
		}
		else
		{
			self.actorsizeto(42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
		}
	}
	void state_botletfallstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 5)
		{
		}
		else
		{
			if (self.actioncounter >= 6)
			{
				self.PlayActorSound("SANTA_TRASH7", CHAN_AUTO, CHANF_SINGULAR);
			}
		}
		if (self.cansee(p))
		{
			setAI('AIFLYWAIT');
			self.ChangeType('NWinterSantaFly');
			return;
		}
		else
		{
			if (self.floorz - self.pos.Z < 50)
			{
				setAI('AIFLYWAIT');
				self.ChangeType('NWinterSantaFly');
				return;
			}
		}
	}
	override void RunState(DukePlayer p, double pdist)
	{
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			if (!self.cansee(p))
			{
				return;
			}
			self.Scale = (42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			setAI('AIBOTWALKSEEK');
			if (self.pal == 21)
			{
				self.extra = MINIBOSSSTRENGTH;
			}
			else
			{
				self.tempval = self.pal;
				self.pal = 0;
			}
			self.PlayActorSound("INSERT_CLIP");
			return;
		}
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.timetosleep += 300;
		if (self.curAction.name == 'ABOTWDEAD')
		{
			return;
		}
		else if (self.curAction.name == 'ABOTWALKFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIBOTWALKHUNT');
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
				if (self.sector != null) self.spawn('DukeAtomicHealth');
				self.PlayActorSound("GLASS_BREAKING");
				if (self.pal == 21)
				{
					state_miniboss(p, pdist);
				}
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
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
		else if (self.curAI == 'AIBOTWALKSTEROIDS')
		{
			state_steroidswalk(p, pdist);
			return;
		}
		else if (self.curAI == 'AIBOTWALKGROW')
		{
			if (self.counter >= 32)
			{
				self.spawnguts('DukeJibs4', 24);
				self.spawnguts('DukeJibs6', 28);
				self.addkill();
				self.PlayActorSound("SQUISHED");
				self.PlayActorSound("PIPEBOMB_EXPLODE");
				self.hitradius(2048, 60, 70, 80, 90);
				if (self.sector != null) self.spawn('DukeBloodPool');
				if (self.sector != null) self.spawn('DukeExplosion2');
				if (self.pal == 21)
				{
					state_miniboss(p, pdist);
				}
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else
			{
				self.actorsizeto(MAXXSTRETCH * REPEAT_SCALE, MAXYSTRETCH * REPEAT_SCALE);
			}
			return;
		}
		else if (self.curAI == 'AIBOTWALKSHRINK')
		{
			if (self.counter >= SHRUNKDONECOUNT)
			{
				setAI('AIBOTWALKHUNT');
			}
			else if (self.counter >= SHRUNKCOUNT)
			{
				if (self.sector != null) self.spawn('DukeFrameEffect');
				self.actorsizeto(42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
			}
			else
			{
				if (self.counter >= 31)
				{
				}
				else
				{
					if (self.counter >= 30)
					{
						if (Duke.rnd(255))
						{
							self.PlayActorSound("PLAYER_TAKEPILLS");
							setAI('AIBOTWALKSTEROIDS');
							return;
						}
					}
				}
				if (self.counter >= 32)
				{
					if (pdist < SQUISHABLEDISTANCE * maptoworld)
					{
						if (self.pal == 0)
						{
							if (Duke.rnd(128))
							{
								self.PlayActorSound("SANTA_DEAD");
							}
							else
							{
								self.PlayActorSound("SANTA_TRASH9");
							}
						}
						state_standard_jibs(p, pdist);
						self.PlayActorSound("SQUISHED");
						if (self.sector != null) self.spawn('DukeBloodPool');
						p.playerkick(self);
						p.wackplayer();
						p.wackplayer();
						p.wackplayer();
						p.wackplayer();
						if (self.pal == 21)
						{
							state_miniboss(p, pdist);
						}
						p.timebeforeexit = 52;
						p.customexitsound = -1;
						ud.eog = true;
						self.killit();
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				else
				{
					self.actorsizeto(MINXSTRETCH * REPEAT_SCALE, MINYSTRETCH * REPEAT_SCALE);
					if (self.sector != null) self.spawn('DukeFrameEffect');
				}
			}
			return;
		}
		else if (self.curAI == 'AIBOTWALKDYING')
		{
			state_botwalkdyingstate(p, pdist);
			return;
		}
		else
		{
			if (Duke.rnd(1))
			{
				self.actoroperate();
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_botwalkhitstate(p, pdist);
			return;
		}
		else if (self.curAI == 'AIWALKFALLHARD')
		{
			if (self.floorz - self.pos.Z < 16)
			{
				setAI('AIBOTWALKHUNT');
			}
			else
			{
				return;
			}
		}
		else
		{
			if (self.curAI == 'AIWALKFALL')
			{
				state_botshootstate(p, pdist);
				if (self.floorz - self.pos.Z < 16)
				{
					setAI('AIBOTWALKHUNT');
				}
				else
				{
					return;
				}
			}
		}
		if (self.checkp(p, pshrunk))
		{
			state_stomphisfreakingass(p, pdist);
		}
		if (self.floorz - self.pos.Z < 550)
		{
		}
		else
		{
			setAI('AIFLYWAIT');
			self.ChangeType('NWinterSantaFly');
		}
		if (abs(self.pos.Z - self.sector.floorz) < 32 && self.sector.lotag == ST_1_ABOVE_WATER)
		{
		}
		else if (self.checkp(p, pjetpack))
		{
			if (self.checkp(p, phigher))
			{
				if (self.cansee(p))
				{
					setAI('AIFLYWAIT');
					self.ChangeType('NWinterSantaFly');
				}
			}
		}
		if (self.checkp(p, phigher))
		{
			if (Duke.rnd(1))
			{
				if (Duke.rnd(196))
				{
					setAI('AIFLYWAIT');
					self.ChangeType('NWinterSantaFly');
				}
			}
		}
		if (self.curAI == 'AIWALKWAIT')
		{
			setAI('AIBOTWALKSEEK');
			return;
		}
		else if (self.curAI == 'AIBOTWALKSEEK')
		{
			state_botwalkseekstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKHUNT')
		{
			state_botwalkhuntstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKJUMP1')
		{
			state_botwalkjumpstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKJUMP2')
		{
			state_botwalkjumpstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKKICK')
		{
			state_botwalkkickstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKSTRAIGHT')
		{
			state_botwalkstraightstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKSTRJUMP1')
		{
			state_botwalkstraightjumpstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKSTRJUMP2')
		{
			state_botwalkstraightjumpstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTLETFALL')
		{
			state_botletfallstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKSTRJUMP1S')
		{
			state_botjumpshootstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTWALKSTRJUMP2S')
		{
			state_botjumpshootstate(p, pdist);
		}
	}
}


class NWinterSantaFly : NWinterSanta
{
	default
	{
		pic "PIGCOPDIVE";	// tiles are offset from here.
		Strength SOBBOTSTRENGTH;
		StartAction "ABOTFLY";
	}
	
	void state_botflydyingstate(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.extra = 0;
		self.cstat = 0;
		if (self.actioncounter >= 5)
		{
			setAction('ABOTFLYDEAD');
			if (self.pal == 21)
			{
				state_miniboss(p, pdist);
			}
			p.timebeforeexit = 52;
			p.customexitsound = -1;
			ud.eog = true;
		}
	}
	void state_choosedodgefly(DukePlayer p, double pdist)
	{
		if (self.pos.Z - self.ceilingz < 100)
		{
			if (self.curAI == 'AIBOTFLYDODGEDOWN')
			{
			}
			else
			{
				setAI('AIBOTFLYDODGEDOWN');
			}
		}
		else
		{
			if (self.curAI == 'AIBOTFLYDODGEUP')
			{
			}
			else
			{
				setAI('AIBOTFLYDODGEUP');
			}
		}
	}
	void state_botflydodgestate(DukePlayer p, double pdist)
	{
		if (self.ifcanshoottarget(p, pdist))
		{
			state_botshootstate(p, pdist);
		}
		if (self.cansee(p))
		{
			if (Duke.rnd(2))
			{
				if (self.pos.Z - self.ceilingz < 100)
				{
					setAI('AIBOTFLYDODGEDOWN');
				}
				else
				{
					setAI('AIBOTFLYDODGEUP');
				}
			}
		}
		else
		{
			setAI('AIBOTFLYSEEKUP');
		}
		if (self.counter >= 10)
		{
			setAI('AIBOTFLYDOWN');
		}
	}
	void state_botflyhitstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (self.sector != null) self.spawn('DukeBlood');
		if (self.extra < 0)
		{
			if (self.pal == 21)
			{
				state_miniboss(p, pdist);
			}
			else
			{
				self.PlayActorSound("SANTA_DEAD");
			}
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.tempval = self.pal;
				self.pal = 1;
				self.extra = 0;
				setMove('none', 0);
				setAction('ABOTFLYFROZEN');
				self.PlayActorSound("SOMETHINGFROZE");
				return;
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.cstat = 0;
				self.PlayActorSound("ACTOR_GROWING");
				setAI('AIBOTFLYGROW');
				return;
			}
			else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.PlayActorSound("SQUISHED");
				self.addkill();
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.PlayActorSound("SQUISHED");
				self.addkill();
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else
			{
				setAI('AIBOTFLYDYING');
			}
		}
		else
		{
			state_dudehurt(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				if (self.curAI == 'AIBOTFLYSTEROIDS')
				{
				}
				else
				{
					setAI('AIBOTFLYSHRINK');
				}
			}
		}
	}
	void state_botflyseekupstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			setAI('AIBOTFLYUP');
			return;
		}
		if (self.checkp(p, phigher))
		{
		}
		else
		{
			setAI('AIBOTFLYSEEKDOWN');
		}
	}
	void state_botflyseekdownstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			setAI('AIBOTFLYDOWN');
			return;
		}
		else
		{
			if (self.checkp(p, phigher))
			{
			}
			else
			{
				setAI('AIBOTLETFALL');
				self.ChangeType('NWinterSanta');
			}
		}
		if (self.checkp(p, phigher))
		{
			setAI('AIBOTFLYSEEKUP');
		}
	}
	void state_botflydownstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (Duke.rnd(16))
		{
			if (self.checkp(p, phigher))
			{
				setAI('AIBOTFLYUP');
				return;
			}
		}
		if (self.cansee(p))
		{
		}
		else
		{
			setAI('AIBOTFLYSTRAIGHT');
		}
	}
	void state_botflyupstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (Duke.rnd(16))
		{
			if (self.checkp(p, phigher))
			{
			}
			else
			{
				setAI('AIBOTFLYDOWN');
				return;
			}
		}
		if (self.cansee(p))
		{
		}
		else
		{
			setAI('AIBOTFLYSTRAIGHT');
		}
	}
	void state_botflyupmomentstate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.counter >= 10)
			{
				setAI('AIBOTFLYUP');
			}
		}
		else
		{
			if (self.counter >= 50)
			{
				setAI('AIBOTLETFALL');
				self.ChangeType('NWinterSanta');
			}
		}
	}
	void state_botflyholdstate(DukePlayer p, double pdist)
	{
		state_botshootstate(p, pdist);
		if (self.counter >= 50)
		{
			setAI('AIBOTFLYUP');
		}
	}
	void state_botflystraightstate(DukePlayer p, double pdist)
	{
		if (pdist > 20000 * maptoworld)
		{
			setAI('AIBOTFLYSEEKDOWN');
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.cansee(p))
		{
			setAI('AIBOTFLYDOWN');
		}
		else
		{
			if (Duke.rnd(128))
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIFLYUPMOMENT');
				}
			}
		}
	}
	void state_steroidsfly(DukePlayer p, double pdist)
	{
		self.PlayActorSound("PLAYER_HARTBEAT", CHAN_AUTO, CHANF_SINGULAR);
		if (self.sector != null) self.spawn('DukeFrameEffect');
		if (self.counter >= 100)
		{
			setAI('AIBOTFLYUP');
		}
		if (self.counter >= 34)
		{
		}
		else
		{
			self.actorsizeto(42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_botflyhitstate(p, pdist);
		}
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			if (!self.cansee(p))
			{
				return;
			}
			if (self.pal == 21)
			{
				self.extra = MINIBOSSSTRENGTH;
			}
			self.Scale = (42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.PlayActorSound("INSERT_CLIP");
			setAI('AIBOTFLYSEEKDOWN');
			self.tempval = self.pal;
			self.pal = 0;
			return;
		}
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		self.timetosleep += 300;
		if (self.curAction.name == 'ABOTFLYDEAD')
		{
			return;
		}
		else if (self.curAction.name == 'ABOTFLYFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIBOTFLYUP');
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
				if (self.sector != null) self.spawn('DukeAtomicHealth');
				self.PlayActorSound("GLASS_BREAKING");
				if (self.pal == 21)
				{
					state_miniboss(p, pdist);
				}
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
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
		else if (self.curAI == 'AIBOTFLYSTEROIDS')
		{
			self.PlayActorSound("PLAYER_JETPACK_IDLE", CHAN_AUTO, CHANF_SINGULAR);
			state_steroidsfly(p, pdist);
			return;
		}
		else if (self.curAI == 'AIBOTFLYGROW')
		{
			if (self.counter >= 32)
			{
				self.spawnguts('DukeJibs4', 24);
				self.spawnguts('DukeJibs6', 28);
				self.addkill();
				self.PlayActorSound("SQUISHED");
				self.PlayActorSound("PIPEBOMB_EXPLODE");
				self.hitradius(2048, 60, 70, 80, 90);
				if (self.sector != null) self.spawn('DukeBloodPool');
				if (self.sector != null) self.spawn('DukeExplosion2');
				if (self.pal == 21)
				{
					state_miniboss(p, pdist);
				}
				p.timebeforeexit = 52;
				p.customexitsound = -1;
				ud.eog = true;
				self.killit();
			}
			else
			{
				self.actorsizeto(MAXXSTRETCH * REPEAT_SCALE, MAXYSTRETCH * REPEAT_SCALE);
				return;
			}
			return;
		}
		else
		{
			if (self.curAI == 'AIBOTFLYSHRINK')
			{
				if (self.counter >= SHRUNKDONECOUNT)
				{
					setAI('AIBOTFLYUP');
				}
				else if (self.counter >= SHRUNKCOUNT)
				{
					if (self.sector != null) self.spawn('DukeFrameEffect');
					self.actorsizeto(42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
				}
				else
				{
					if (self.counter >= 31)
					{
					}
					else
					{
						if (self.counter >= 30)
						{
							if (Duke.rnd(255))
							{
								self.PlayActorSound("PLAYER_TAKEPILLS");
								setAI('AIBOTFLYSTEROIDS');
								return;
							}
						}
					}
					if (self.counter >= 32)
					{
						if (pdist < SQUISHABLEDISTANCE * maptoworld)
						{
							if (self.pal == 0)
							{
								if (Duke.rnd(128))
								{
									self.PlayActorSound("SANTA_DEAD");
								}
								else
								{
									self.PlayActorSound("SANTA_DEAD");
								}
							}
							state_standard_jibs(p, pdist);
							state_standard_jibs(p, pdist);
							state_standard_jibs(p, pdist);
							state_standard_jibs(p, pdist);
							self.PlayActorSound("SQUISHED");
							if (self.sector != null) self.spawn('DukeBloodPool');
							p.playerkick(self);
							p.wackplayer();
							p.wackplayer();
							p.wackplayer();
							p.wackplayer();
							if (self.pal == 21)
							{
								state_miniboss(p, pdist);
							}
							p.timebeforeexit = 52;
							p.customexitsound = -1;
							ud.eog = true;
							self.killit();
						}
						if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
					}
					else
					{
						self.actorsizeto(MINXSTRETCH * REPEAT_SCALE, MINYSTRETCH * REPEAT_SCALE);
						if (self.sector != null) self.spawn('DukeFrameEffect');
					}
				}
				return;
			}
		}
		self.PlayActorSound("PLAYER_JETPACK_IDLE", CHAN_AUTO, CHANF_SINGULAR);
		if (self.curAI == 'AIBOTFLYDYING')
		{
			state_botflydyingstate(p, pdist);
			return;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_botflyhitstate(p, pdist);
		}
		if (Duke.rnd(2))
		{
			self.actoroperate();
		}
		if (self.checkp(p, pshrunk))
		{
			state_stomphisfreakingass(p, pdist);
		}
		if (Duke.rnd(25))
		{
			if (self.checkp(p, phigher))
			{
			}
			else if (self.checkp(p, ponground))
			{
				if (self.floorz - self.pos.Z < 200)
				{
					self.PlayActorSound("PLAYER_JETPACK_OFF");
					if (self.floorz - self.pos.Z < 100)
					{
						setAI('AIWALKFALL');
					}
					else
					{
						setAI('AIWALKFALLHARD');
					}
					self.ChangeType('NWinterSanta');
				}
			}
		}
		if (self.curAI == 'AIFLYWAIT')
		{
			setMove('none', 0);
			self.PlayActorSound("PLAYER_JETPACK_ON");
			setAI('AIBOTFLYSEEKDOWN');
		}
		if (self.curAI == 'AIBOTFLYSEEKUP')
		{
			state_botflyseekupstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYSEEKDOWN')
		{
			state_botflyseekdownstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYDOWN')
		{
			state_botflydownstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYUP')
		{
			state_botflyupstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYDODGEUP')
		{
			state_botflydodgestate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYDODGEDOWN')
		{
			state_botflydodgestate(p, pdist);
		}
		else if (self.curAI == 'AIFLYUPMOMENT')
		{
			state_botflyupmomentstate(p, pdist);
		}
		else if (self.curAI == 'AIFLYUPHOLD')
		{
			state_botflyholdstate(p, pdist);
		}
		else if (self.curAI == 'AIBOTFLYSTRAIGHT')
		{
			state_botflystraightstate(p, pdist);
		}
	}
	
}
