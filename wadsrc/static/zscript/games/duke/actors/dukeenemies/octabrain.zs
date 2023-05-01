class DukeOctabrain : DukeActor
{
	const OCTASTRENGTH = 175;
	const OCTASCRATCHINGPLAYER = -11;
	
	default
	{
		pic "OCTABRAIN";
		Strength OCTASTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		falladjustz 0;
		
		action "ALIZWALKING", 0, 4, 5, 1, 15;
		move "LIZWALKVEL", 72;
		ai "AILIZGETENEMY", "ALIZWALKING", "LIZWALKVEL", seekplayer;
		action "AOCTAWALK", 0, 3, 5, 1, 15;
		action "AOCTASTAND", 0, 1, 5, 1, 15;
		action "AOCTASCRATCH", 0, 4, 5, 1, 15;
		action "AOCTAHIT", 30, 1, 1, 1, 10;
		action "AOCTASHOOT", 20, 1, 5, 1, 10;
		action "AOCTADYING", 30, 8, 1, 1, 17;
		action "AOCTADEAD", 38, 1, 1, 1, 1;
		action "AOCTAFROZEN", 0, 1, 5;
		move "OCTAWALKVELS", 96, -30;
		move "OCTAUPVELS", 96, -70;
		move "OCTASTOPPED", 0, -30;
		move "OCTAINWATER", 96, 24;
		ai "AIOCTAGETENEMY", "AOCTAWALK", "OCTAWALKVELS", seekplayer;
		ai "AIOCTASHOOTENEMY", "AOCTASHOOT", "OCTASTOPPED", faceplayer;
		ai "AIOCTASCRATCHENEMY", "AOCTASCRATCH", "OCTASTOPPED", faceplayerslow;
		ai "AIOCTAHIT", "AOCTAHIT", "OCTASTOPPED", faceplayer;
		ai "AIOCTASHRUNK", "AOCTAWALK", "SHRUNKVELS", faceplayer;
		ai "AIOCTAGROW", "AOCTASTAND", "OCTASTOPPED", faceplayerslow;
		ai "AIOCTADYING", "AOCTADYING", "OCTASTOPPED", faceplayer;
		ai "AIOCTAONFIRE", "AOCTAWALK", "OCTAWALKVELS", fleeenemy;
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("OCTA_RECOG");
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_octaonfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			setAI('AIOCTAGETENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("OCTA_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			setAI('AIOCTADYING');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_octagetenemystate(DukePlayer p, double pdist)
	{
		if (self.cansee(p))
		{
			if (self.actioncounter >= 32)
			{
				if (Duke.rnd(48))
				{
					if (self.ifcanshoottarget(p, pdist))
					{
						self.PlayActorSound("OCTA_ATTACK1");
						setAI('AIOCTASHOOTENEMY');
					}
				}
			}
			else
			{
				if (pdist < 1280 * maptoworld)
				{
					setAI('AIOCTASCRATCHENEMY');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_octascratchenemystate(DukePlayer p, double pdist)
	{
		if (pdist > 1280 * maptoworld)
		{
			setAI('AIOCTAGETENEMY');
		}
		else
		{
			if (self.counter >= 32)
			{
				self.counter = 0;
				self.PlayActorSound("OCTA_ATTACK2");
				p.pals = color(8, 32, 0, 0);
				p.addphealth(OCTASCRATCHINGPLAYER, false);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_octashootenemystate(DukePlayer p, double pdist)
	{
		if (self.counter >= 25)
		{
			if (self.counter >= 27)
			{
				setAI('AIOCTAGETENEMY');
			}
		}
		else if (self.counter >= 24)
		{
			self.shoot('DukeCoolExplosion1');
		}
		else
		{
			if (self.actioncounter >= 6)
			{
				self.actioncounter = 0;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkoctahitstate(DukePlayer p, double pdist)
	{
		if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
		{
			if (self.pal == 2)
			{
				self.pal = self.tempval;
				self.tempval = 0;
			}
			self.PlayActorSound("ACTOR_SHRINKING");
			setAI('AIOCTASHRUNK');
		}
		else
		{
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.PlayActorSound("SOMETHINGFROZE");
					self.tempval = self.pal;
					self.pal = 1;
					setMove('none', 0);
					setAction('AOCTAFROZEN');
					self.extra = 0;
					return;
				}
				self.addkill();
				if (self.attackertype.GetClassName() == 'DukeRPG')
				{
					self.PlayActorSound("SQUISHED");
					state_standard_jibs(p, pdist);
					self.killit();
				}
				else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					self.PlayActorSound("SQUISHED");
					state_standard_jibs(p, pdist);
					self.killit();
				}
				else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					self.cstat = 0;
					self.PlayActorSound("ACTOR_GROWING");
					setAI('AIOCTAGROW');
					return;
				}
				else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
				{
					state_spawnburnedcorpse(p, pdist);
				}
				else
				{
					state_rf(p, pdist);
					setAI('AIOCTADYING');
				}
				self.PlayActorSound("OCTA_DYING");
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeRPG')
				{
					self.PlayActorSound("OCTA_DYING");
					self.addkill();
					state_standard_jibs(p, pdist);
					self.killit();
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeGrowSpark')
					{
						self.PlayActorSound("EXPANDERHIT");
					}
				}
				if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
				{
					if (self.pal == 2)
					{
					}
					else
					{
						self.tempval = self.pal;
						self.pal = 2;
						setAI('AIOCTAONFIRE');
						self.counter = 0;
						state_octaonfirestate(p, pdist);
					}
				}
				else if (self.curAI == 'AIOCTAONFIRE')
				{
					return;
				}
				else
				{
					self.PlayActorSound("OCTA_PAIN");
					if (self.sector != null) self.spawn('DukeBlood');
					if (Duke.rnd(64))
					{
						setAI('AIOCTAHIT');
					}
				}
			}
		}
		if (self.curAI != 'AIOCTAONFIRE')
		{
			state_random_wall_jibs(p, pdist);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_octashrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			setAI(IsVacation()? 'AIOCTAGETENEMY' : 'AILIZGETENEMY'); // Vaca fixed this. Everything else has the wrong one.
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

	void state_octadyingstate(DukePlayer p, double pdist)
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
				if (Duke.rnd(64))
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
			}
			setMove('OCTASTOPPED', 0);
			setAction('AOCTADEAD');
		}
		else if (self.actioncounter == 4)
		{
			if (self.floorz - self.pos.Z < 8)
			{
				self.PlayActorSound("THUD");
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
		state_checksquished(p, pdist);
		if (self.curAI == 'none')
		{
			setAI('AIOCTAGETENEMY');
		}
		else if (self.curAction.name == 'AOCTADEAD')
		{
			self.extra = 0;
			if (self.counter >= RESPAWNACTORTIME)
			{
				if (ud.respawn_monsters) // for enemies:
				{
					self.subkill();
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = OCTASTRENGTH;
					setAI('AIOCTAGETENEMY');
				}
			}
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					state_standard_jibs(p, pdist);
					self.killit();
				}
			}
			return;
		}
		else if (self.curAction.name == 'AOCTAFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAI('AIOCTAGETENEMY');
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
				self.addkill();
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.extra = 0;
					return;
				}
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
		else
		{
			if (Duke.rnd(1))
			{
				self.PlayActorSound("OCTA_ROAM", CHAN_AUTO, CHANF_SINGULAR);
			}
			if (self.curAI == 'AIOCTAGETENEMY')
			{
				state_octagetenemystate(p, pdist);
			}
			else if (self.curAI == 'AIOCTAHIT')
			{
				if (self.counter >= 8)
				{
					setAI('AIOCTASHOOTENEMY');
				}
			}
			else if (self.curAI == 'AIOCTADYING')
			{
				state_octadyingstate(p, pdist);
				return;
			}
			else if (self.curAI == 'AIOCTASCRATCHENEMY')
			{
				state_octascratchenemystate(p, pdist);
			}
			else if (self.curAI == 'AIOCTASHOOTENEMY')
			{
				state_octashootenemystate(p, pdist);
			}
			else if (self.curAI == 'AIOCTASHRUNK')
			{
				state_octashrunkstate(p, pdist);
				return;
			}
			else if (self.curAI == 'AIOCTAGROW')
			{
				state_genericgrowcode(p, pdist);
			}
			else if (self.curAI == 'AIOCTAONFIRE')
			{
				state_octaonfirestate(p, pdist);
			}
			if (self.curMove.name == 'OCTAUPVELS')
			{
			}
			else if (self.checkp(p, phigher))
			{
				setMove('OCTAUPVELS', seekplayer);
			}
			else if (self.curMove.name != 'OCTAINWATER')
			{
				if (self.sector.lotag == ST_2_UNDERWATER)
				{
					setMove('OCTAINWATER', seekplayer);
				}
			}
			if (self.ifhitbyweapon() >= 0)
			{
				state_checkoctahitstate(p, pdist);
			}
		}
	}
	
}

class DukeOctabrainStayput: DukeOctabrain
{
	default
	{
		pic "OCTABRAINSTAYPUT";
		+DONTDIVEALIVE;
		+BADGUYSTAYPUT;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		setAI('AIOCTAGETENEMY');
		self.ChangeType('DukeOctabrain');
	}
	
}

