class DukeFemBase : DukeActor
{
	meta int femflags;
	property femflags: femflags;
	
	default
	{
		+TRIGGERRESPAWN;
		+HITRADIUSCHECK;
		//StartAction "FEMANIMATE";

		action "FEMSHRUNK", 0;
		action "FEMFROZEN1", 1;
		action "FEMGROW", 0;
		action "FEMFROZEN2", 0;
		action "FEMDANCE1", 19, 1, 1, 1, 16;
		action "FEMDANCE3", 19, 1, 1, 1, 26;
		action "FEMDANCE2", 20, 2, 1, 1, 10;
		action "FEMANIMATESLOW", 0, 2, 1, 1, 100;
		action "TOUGHGALANIM", 0, 5, 1, 1, 25;
		action "FEMANIMATE", 0;
		
		Strength TOUGH;
	}		

	const GROWSCRAP = 1;
	const SLOWANIM = 2;
	const TOUGHANIM = 4;
	const TIPME = 8;
	const KILLME = 16;
	const FEMDANCE = 32;
	const FREEZEANIM2 = 64;
	

	override void Initialize()
	{
		self.yint = self.hitag;
		self.hitag = -1;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
	
	void restartAnim()
	{
		if (self.detail & TOUGHANIM)
		{
			setAction('TOUGHGALANIM');
		}
		else if (self.detail & SLOWANIM)
		{
			setAction('FEMANIMATESLOW');
		}
		else
		{
			setAction('FEMANIMATE');
		}
	}
	
	void state_killme(DukePlayer p, double pdist)
	{
		if (self.sector.lotag == ST_2_UNDERWATER)
		{
		}
		else
		{
			if (self.checkp(p, pfacing))
			{
				if (pdist < 1280 * maptoworld)
				{
					if (p.PlayerInput(Duke.SB_OPEN))
					{
						self.PlayActorSound("KILLME", CHAN_AUTO, CHANF_SINGULAR);
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
	void state_tipme(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pfacing))
		{
			if (pdist < 1280 * maptoworld)
			{
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					p.tipincs = 26;
					if (Duke.rnd(128))
					{
						self.PlayActorSound("PLAYER_TIP1", CHAN_AUTO, CHANF_SINGULAR);
					}
					else
					{
						self.PlayActorSound("PLAYER_TIP2", CHAN_AUTO, CHANF_SINGULAR);
					}
					if (self.detail & FEMDANCE)
					{
						setAction('FEMDANCE1');
					}
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.tempval = self.pal;
		self.pal = 6;
		self.PlayActorSound("LADY_SCREAM", CHAN_AUTO, CHANF_SINGULAR);
		if (self.detail & GROWSCRAP)
		{
			self.spawndebris(DukeScrap.Scrap3, 18);
		}
		self.killit();
	}
	
	override void BrkHit()
	{
		self.spawnguts('DukeJibs6', 1);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.detail & TIPME) state_tipme(p, pdist);
		if (self.detail & KILLME) state_killme(p, pdist);
		self.fall(p);
		if (self.curAction.name == 'FEMSHRUNK')
		{
			if (self.counter >= SHRUNKDONECOUNT)
			{
				setAction('FEMANIMATE');
				self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
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
		else if (self.curAction.name == 'FEMGROW')
		{
			if (self.counter >= 32)
			{
				self.respawnhitag();
				self.spawnguts('DukeJibs4', 20);
				self.spawnguts('DukeJibs6', 20);
				self.tempval = self.pal;
				self.pal = 6;
				self.PlayActorSound("LADY_SCREAM", CHAN_AUTO, CHANF_SINGULAR);
				if (self.detail & GROWSCRAP)
				{
					self.spawndebris(DukeScrap.Scrap3, 4);
				}
				self.PlayActorSound("SQUISHED");
				self.killit();
			}
			else
			{
				self.actorsizeto(MAXXSTRETCH * REPEAT_SCALE, MAXYSTRETCH * REPEAT_SCALE);
			}
		}
		else if (self.curAction.name == 'FEMDANCE1')
		{
			if (self.actioncounter >= 2)
			{
				setAction('FEMDANCE2');
			}
		}
		else if (self.curAction.name == 'FEMDANCE2')
		{
			if (self.actioncounter >= 8)
			{
				setAction('FEMDANCE3');
			}
		}
		else if (self.curAction.name == 'FEMDANCE3')
		{
			if (self.actioncounter >= 2)
			{
				setAction('FEMANIMATE');
			}
		}
		else if (self.curAction.name == 'FEMFROZEN1')
		{
			if (self.counter >= THAWTIME)
			{
				restartAnim();
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
				self.spriteglass(30);
				self.PlayActorSound("GLASS_BREAKING");
				self.respawnhitag();
				if (Duke.rnd(84))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
				self.killit();
			}
			else
			{
				if (self.checkp(p, pfacing))
				{
					if (pdist < FROZENQUICKKICKDIST * maptoworld)
					{
						p.playerkick(self);
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
			return;
		}
		else
		{
			if (self.curAction.name == 'FEMFROZEN2')
			{
				if (self.counter >= THAWTIME)
				{
					restartAnim();
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
					self.spriteglass(30);
					self.PlayActorSound("GLASS_BREAKING");
					if (Duke.rnd(84))
					{
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					self.respawnhitag();
					if (Duke.rnd(128))
					{
						self.PlayActorSound("PLAYER_HIT_STRIPPER1");
					}
					else
					{
						self.PlayActorSound("PLAYER_HIT_STRIPPER2");
					}
					self.killit();
				}
				else
				{
					if (self.checkp(p, pfacing))
					{
						if (pdist < FROZENQUICKKICKDIST * maptoworld)
						{
							p.playerkick(self);
						}
						if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
					}
				}
				return;
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.cstat = 0;
					setMove('none', 0);
					self.PlayActorSound("ACTOR_GROWING");
					setAction('FEMGROW');
					return;
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
					{
						if (self.curAction.name == 'FEMSHRUNK')
						{
							return;
						}
						if (detail & FREEZEANIM2) setAction('FEMFROZEN2');
						else setAction('FEMFROZEN1');
						setMove('none', 0);
						self.tempval = self.pal;
						self.pal = 1;
						self.extra = 0;
						self.PlayActorSound("SOMETHINGFROZE");
						return;
					}
				}
				if (Duke.rnd(128))
				{
					self.PlayActorSound("PLAYER_HIT_STRIPPER1");
				}
				else
				{
					self.PlayActorSound("PLAYER_HIT_STRIPPER2");
				}
				self.respawnhitag();
				state_standard_jibs(p, pdist);
				state_random_wall_jibs(p, pdist);
				if (self.sector != null) self.spawn('DukeBloodPool');
				BrkKilled(p, pdist);
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					self.PlayActorSound("ACTOR_SHRINKING");
					setMove('none', 0);
					setAction('FEMSHRUNK');
					self.cstat = 0;
					return;
				}
				else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.PlayActorSound("EXPANDERHIT");
				}
				BrkHit();
			}
		}
	}
}


class DukeBloodyPole : DukeFemBase
{
	default
	{
		pic "BLOODYPOLE";
		-TRIGGERRESPAWN;
		-HITRADIUSCHECK;
		StartAction 'none';
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.fall(p);
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				state_standard_jibs(p, pdist);
				self.killit();
			}
		}
	}
	
}

class DukeFemale1 : DukeFemBase
{
	default
	{
		pic "FEM1";
		DukeFemBase.femflags TIPME | FEMDANCE;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.lotsofstuff('DukeMoney', 5);
	}
}
		
class DukeFemale2 : DukeFemBase
{
	default
	{
		pic "FEM2";
		DukeFemBase.femflags TIPME;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.lotsofstuff('DukeMoney', 7);
		self.ChangeType('DukeBarBroke');
		self.cstat = 0;
	}
	
}
	
class DukeFemale3 : DukeFemBase
{
	default
	{
		pic "FEM3";
		DukeFemBase.femflags TIPME;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.lotsofstuff('DukeMoney', 4);
	}
	
}
	
class DukeFemale4 : DukeFemBase
{
	default
	{
		pic "FEM4";
	}
}
	
class DukeFemale5 : DukeFemBase
{
	default
	{
		pic "FEM5";
		DukeFemBase.femflags FREEZEANIM2 | KILLME;
	}
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.extra = TOUGH;
		self.ChangeType('DukeBloodyPole');
	}
}
	
class DukeFemale6 : DukeFemBase
{
	default
	{
		pic "FEM6";
		+NOGRAVITY;
		DukeFemBase.femflags FREEZEANIM2 | KILLME;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.cstat = 0;
		self.ChangeType('DukeFem6Pad');
	}
	
}
	
class DukeFemale7 : DukeFemBase
{
	default
	{
		pic "FEM7";
		DukeFemBase.femflags TIPME;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.lotsofstuff('DukeMoney', 8);
	}
	
}
	
class DukeFemale8 : DukeFemBase
{
	default
	{
		pic "FEM8";
		DukeFemBase.femflags FREEZEANIM2;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.extra = TOUGH;
		self.ChangeType('DukeBloodyPole');
	}
	
	override void BrkHit(){}
	
}
	
class DukeFemale9 : DukeFemBase
{
	default
	{
		pic "FEM9";
		DukeFemBase.femflags FREEZEANIM2;
	}
}
	
class DukeFemale10 : DukeFemBase
{
	default
	{
		pic "FEM10";
		DukeFemBase.femflags SLOWANIM | FREEZEANIM2 | TIPME;
		StartAction "FEMANIMATESLOW";
	}
}
	
class DukeNaked : DukeFemBase
{
	default
	{
		pic "NAKED1";
		-HITRADIUSCHECK;
		+NOGRAVITY;
		DukeFemBase.femflags GROWSCRAP | FREEZEANIM2 | KILLME;
	}
}
	
class DukeToughGal : DukeFemBase
{
	const MANWOMANSTRENGTH = 100;
	default
	{
		pic "TOUGHGAL";
		-HITRADIUSCHECK;
		Strength MANWOMANSTRENGTH;
		DukeFemBase.femflags TOUGHANIM | FREEZEANIM2;
		StartAction "TOUGHGALANIM";
	}
	
	void state_toughgaltalk(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pfacing))
		{
			if (pdist < 1280 * maptoworld)
			{
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					BrkHit();
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
	}
	
	override void BrkHit()
	{
		if (self.CheckAnyActorSoundPlaying()) return;
		if (Duke.rnd(64))
		{
			self.PlayActorSound("TOUGHGALSND1", CHAN_AUTO, CHANF_SINGULAR);
		}
		else if (Duke.rnd(64))
		{
			self.PlayActorSound("TOUGHGALSND2", CHAN_AUTO, CHANF_SINGULAR);
		}
		else if (Duke.rnd(64))
		{
			self.PlayActorSound("TOUGHGALSND3", CHAN_AUTO, CHANF_SINGULAR);
		}
		else
		{
			self.PlayActorSound("TOUGHGALSND4", CHAN_AUTO, CHANF_SINGULAR);
		}
	}
	
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_toughgaltalk(p, pdist);
		Super.RunState(p, pdist);
	}
}

class DukePodFemale : DukeFemBase
{
	default
	{
		pic "PODFEM1";
		DukeFemBase.femflags GROWSCRAP | FREEZEANIM2 | KILLME;
	}
	
	override void Initialize()
	{
		Super.Initialize();
		self.extra <<= 1;
	}
	
}

class DukeFem6Pad: DukeActor
{
	default
	{
		pic "FEM6PAD";
	}
}
