class DukePlayerPawn : DukePlayerBase
{
	default
	{
		action "PGROWING", 0;
		action "PSTAND", 0, 1, 5, 1, 1;
		action "PEXPLODE", 106, 5, 1, 1, 10;
		action "PEXPLODEAD", 113, 1, 1;
		action "PJPHOUVER", 15, 1, 5, 1;
		action "PWALK", 20, 4, 5, 1, 16;
		action "PRUN", 20, 4, 5, 1, 10;
		action "PWALKBACK", 45, 4, 5, -1, 16;
		action "PRUNBACK", 45, 4, 5, -1, 10;
		action "PJUMPING", 50, 4, 5, 1, 30;
		action "PFALLING", 65, 1, 5;
		action "PDUCKING", 86, 1, 5;
		action "PCRAWLING", 86, 3, 5, 1, 20;
		action "PAKICKING", 40, 2, 5, 1, 25;
		action "PFLINTCHING", 106, 1, 1, 1, 10;
		action "PTHROWNBACK", 106, 5, 1, 1, 18;
		action "PFROZEN", 20, 1, 5;
		action "PLYINGDEAD", 113, 1, 1;
		action "PONFIRE", 20, 4, 5, 1, 16;
		action "PSWIMMINGGO", 375, 1, 5, 1, 10;
		action "PSWIMMING", 375, 4, 5, 1, 13;
		action "PSWIMMINGWAIT", 395, 1, 5, 1, 13;
		action "PTREDWATER", 395, 2, 5, 1, 17;
		move "PSTOPED";
		move "PSHRINKING";
		move "PGROWINGPOP";
		Strength MAXPLAYERHEALTH;
		StartAction "PSTAND";
		StartMove "none";
		moveflags 0;
	}
	
	void state_headhitstate(DukePlayer p, double pdist)
	{
	}

	void state_check_pstandard(DukePlayer p, double pdist)
	{
		if (self.checkp(p, pwalking))
		{
			setAction('PWALK');
		}
		else if (self.checkp(p, pkicking))
		{
			setAction('PAKICKING');
		}
		else if (self.checkp(p, pwalkingback))
		{
			setAction('PWALKBACK');
		}
		else if (self.checkp(p, prunning))
		{
			setAction('PRUN');
		}
		else if (self.checkp(p, prunningback))
		{
			setAction('PRUNBACK');
		}
		else if (self.checkp(p, pjumping))
		{
			setAction('PJUMPING');
		}
		else if (self.checkp(p, pducking))
		{
			setAction('PDUCKING');
		}
	}
	
	void state_standard_pjibs(DukePlayer p, double pdist)
	{
		self.spawnguts('DukeJibs1', 1);
		self.spawnguts('DukeJibs3', 2);
		self.spawnguts('DukeJibs4', 1);
		self.spawnguts('DukeJibs5', 1);
		self.spawnguts('DukeJibs6', 2);
		self.spawnguts('DukePlayerTorso', 1);
		self.spawnguts('DukePlayerLeg', 2);
		self.spawnguts('DukePlayerGun', 1);
		if (Duke.rnd(16))
		{
			self.lotsofstuff('DukeMoney', 1);
		}
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('PSTAND');
		}
		if (self.curAction.name == 'PONFIRE')
		{
			p.pals = color(32, 64, 32, 0);
			if (abs(self.pos.Z - self.sector.floorz) < 32 && self.sector.lotag == ST_1_ABOVE_WATER)
			{
				self.counter = ONFIRETIME;
			}
			else if (self.sector.lotag == ST_2_UNDERWATER)
			{
				self.counter = ONFIRETIME;
			}
			if (self.counter >= ONFIRETIME)
			{
				self.pal = self.GetPlayer().palookup;
				setMove('none', 0);
				setAction('PSTAND');
			}
			else
			{
				if (Duke.rnd(26))
				{
					self.extra += -3;
					self.PlayActorSound("FLESH_BURNING");
				}
				if (Duke.rnd(128))
				{
					if (self.sector != null) self.spawn('DukeOnFireSmoke');
				}
			}
			if (ud.multimode > 1)
			{
				if (self.extra <= 0) // ifstrength
				{
					self.extra = 0;
					if (self.ifhitbyweapon() >= 0)
					{
					}
				}
			}
		}
		if (self.curAction.name == 'PFROZEN')
		{
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			p.pals = color(16, 0, 0, 24);
			if (self.curMove.name == 'none')
			{
				if (self.ifhitbyweapon() >= 0)
				{
					if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
					{
						return;
					}
					self.spriteglass(60);
					if (Duke.rnd(84))
					{
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					self.PlayActorSound("GLASS_BREAKING");
					if (self.sector != null) self.spawn('DukeAtomicHealth');
					self.pal = self.GetPlayer().palookup;
					setMove('PSTOPED', 0);
					return;
				}
			}
			else
			{
				self.cstat = CSTAT_SPRITE_INVISIBLE;
				p.FTA(13);
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					setAction('PSTAND');
					p.playerreset(self);
				}
				return;
			}
			if (self.actioncounter >= THAWTIME)
			{
				self.pal = self.GetPlayer().palookup;
				self.extra = 1;
				setMove('none', 0);
				setAction('PSTAND');
			}
			else
			{
				if (self.actioncounter >= FROZENDRIPTIME)
				{
					if (Duke.rnd(32))
					{
						if (self.sector != null) self.spawn('DukeWaterDrip');
					}
				}
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
		if (self.extra <= 0)
		{
			if (self.curAction.name == 'PGROWING')
			{
				if (self.curMove.name == 'PGROWINGPOP')
				{
					p.FTA(13);
					if (p.PlayerInput(Duke.SB_OPEN))
					{
						setAction('none');
						p.playerreset(self);
					}
					return;
				}
				else
				{
					if (self.counter >= 32)
					{
						self.PlayActorSound("SQUISHED");
						p.pals = color(48, 64, 0, 0);
						state_standard_pjibs(p, pdist);
						self.spawnguts('DukeJibs4', 20);
						self.spawnguts('DukeJibs6', 20);
						setMove('PGROWINGPOP', 0);
						self.cstat = CSTAT_SPRITE_INVISIBLE;
						self.GetPlayer().checkweapons();
						self.hitradius(2048, 60, 70, 80, 90);
					}
					else
					{
						self.actorsizeto(MAXXSTRETCH * REPEAT_SCALE, MAXYSTRETCH * REPEAT_SCALE);
					}
				}
				return;
			}
			if (self.ifsquished(p))
			{
				p.pals = color(32, 63, 63, 63);
			}
			else
			{
				self.xoffset = self.yoffset = 0;
				self.fall(p);
			}
			if (self.actioncounter >= 7)
			{
				setMove('none', 0);
			}
			else
			{
				if (self.actioncounter >= 6)
				{
					if (ud.multimode > 1)
					{
					}
					else
					{
						if (Duke.rnd(32))
						{
							self.PlayActorSound("PLAYER_KILLED5");
						}
						else
							if (Duke.rnd(32))
							{
								self.PlayActorSound("PLAYER_KILLED3");
							}
						else
							if (Duke.rnd(32))
							{
								self.PlayActorSound("PLAYER_KILLED1");
							}
						else
							if (Duke.rnd(32))
							{
								self.PlayActorSound("PLAYER_KILLED2");
							}
					}
				}
			}
			if (self.curAction.name == 'PLYINGDEAD')
			{
				if (self.actioncounter >= 3)
				{
					setMove('PSTOPED', 0);
				}
				p.FTA(13);
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					setAction('PSTAND');
					if (self.sector != null) self.spawn('DukePlayerLyingDead');
					p.playerreset(self);
				}
				return;
			}
			if (self.curAction.name == 'PTHROWNBACK')
			{
				if (self.actioncounter >= 5)
				{
					if (self.sector != null) self.spawn('DukeBloodPool');
					setAction('PLYINGDEAD');
				}
				else
				{
					if (self.actioncounter >= 1)
					{
						setMove('none', 0);
					}
				}
				return;
			}
			if (self.curAction.name == 'PEXPLODEAD')
			{
				p.FTA(13);
				if (p.PlayerInput(Duke.SB_OPEN))
				{
					p.playerreset(self);
					if (ud.multimode > 1) setAction('PSTAND');
				}
				return;
			}
			if (self.curAction.name == 'PEXPLODE')
			{
				if (self.actioncounter >= 5)
				{
					setAction('PEXPLODEAD');
					if (self.sector != null) self.spawn('DukeBloodPool');
				}
				return;
			}
			if (self.checkp(p, pshrunk))
			{
				state_standard_pjibs(p, pdist);
				if (self.sector != null) self.spawn('DukeBloodPool');
				self.PlayActorSound("SQUISHED");
				self.PlayActorSound("PLAYER_DEAD");
				self.cstat = CSTAT_SPRITE_INVISIBLE;
				setAction('PLYINGDEAD');
			}
			else
			{
				if (self.sector.lotag == ST_2_UNDERWATER)
				{
					setAction('PLYINGDEAD');
					if (self.sector != null) self.spawn('DukeWaterBubble');
					if (self.sector != null) self.spawn('DukeWaterBubble');
				}
				else
				{
					setAction('PEXPLODE');
					state_standard_pjibs(p, pdist);
					self.cstat = CSTAT_SPRITE_INVISIBLE;
					self.PlayActorSound("SQUISHED");
					self.PlayActorSound("PLAYER_DEAD");
				}
			}
			self.GetPlayer().checkweapons();
			return;
		}
		if (self.ifsquished(p))
		{
			self.extra = -1;
			self.PlayActorSound("SQUISHED");
			state_random_ooz(p, pdist);
			return;
		}
		if (self.checkp(p, ponsteroids))
		{
			if (self.checkp(p, pstanding))
			{
			}
			else
			{
				if (self.sector != null) self.spawn('DukeFrameEffect');
			}
		}
		if (self.curMove.name == 'PSHRINKING')
		{
			if (self.counter >= 32)
			{
				if (self.counter >= SHRUNKDONECOUNT)
				{
					setMove('none', 0);
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
				}
				else
					if (self.counter >= SHRUNKCOUNT)
					{
						self.actorsizeto(42 * REPEAT_SCALE, 36 * REPEAT_SCALE);
						if (self.floorz - self.ceilingz < 24)
						{
							self.extra = 0;
							self.PlayActorSound("SQUISHED");
							p.pals = color(48, 64, 0, 0);
							return;
						}
					}
				else
				{
					if (self.checkp(p, ponsteroids))
					{
						self.counter = SHRUNKCOUNT;
					}
				}
			}
			else
			{
				if (self.checkp(p, ponsteroids))
				{
					self.counter = SHRUNKCOUNT;
				}
				else
				{
					self.actorsizeto(8 * REPEAT_SCALE, 9 * REPEAT_SCALE);
					if (self.sector != null) self.spawn('DukeFrameEffect');
				}
			}
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.extra <= 0)
				{
					if (ud.multimode > 1)
					{
						self.PlayActorSound("PLAYER_KILLED4");
					}
					if (self.attackertype.GetClassName() == 'DukeGrowSpark')
					{
						p.pals = color(48, 48, 0, 0);
						setAction('PGROWING');
						self.counter = 0;
						setMove('none', 0);
						self.PlayActorSound("ACTOR_GROWING");
						self.cstat = 0;
						return;
					}
				}
				else
				{
					if (ud.multimode > 1)
					{
						if (p.actor.extra < YELLHURTSOUNDSTRENGTHMP)
						{
							if (Duke.rnd(64))
							{
								self.PlayActorSound("PLAYER_LONGTERM_PAIN2");
							}
							else
								if (Duke.rnd(64))
								{
									self.PlayActorSound("PLAYER_LONGTERM_PAIN3");
								}
							else
								if (Duke.rnd(64))
								{
									self.PlayActorSound("PLAYER_LONGTERM_PAIN4");
								}
							else
							{
								self.PlayActorSound("PLAYER_DEAD");
							}
						}
						else
						{
							if (Duke.rnd(64))
							{
								self.PlayActorSound("PLAYER_LONGTERM_PAIN5");
							}
							else
								if (Duke.rnd(64))
								{
									self.PlayActorSound("PLAYER_LONGTERM_PAIN6");
								}
							else
								if (Duke.rnd(64))
								{
									self.PlayActorSound("PLAYER_LONGTERM_PAIN7");
								}
							else
							{
								self.PlayActorSound("PLAYER_LONGTERM_PAIN8");
							}
						}
					}
					else
					{
						if (p.actor.extra < YELLHURTSOUNDSTRENGTH)
						{
							if (Duke.rnd(74))
							{
								self.PlayActorSound("PLAYER_LONGTERM_PAIN2");
							}
							else
								if (Duke.rnd(8))
								{
									self.PlayActorSound("PLAYER_LONGTERM_PAIN3");
								}
							else
							{
								self.PlayActorSound("PLAYER_LONGTERM_PAIN4");
							}
						}
						if (Duke.rnd(128))
						{
							self.PlayActorSound("PLAYER_LONGTERM_PAIN");
						}
					}
				}
				if (self.extra <= TOUGH) // ifstrength
				{
					state_headhitstate(p, pdist);
					self.PlayActorSound("PLAYER_GRUNT");
					if (self.checkp(p, pstanding))
					{
						setAction('PFLINTCHING');
					}
				}
				if (self.attackertype.GetClassName() == 'DukeRPG')
				{
					if (Duke.rnd(32))
					{
						if (self.sector != null) self.spawn('DukeBlood');
					}
					if (self.extra <= 0)
					{
						state_standard_pjibs(p, pdist);
					}
					p.pals = color(48, 52, 0, 0);
					return;
				}
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					if (Duke.rnd(32))
					{
						if (self.sector != null) self.spawn('DukeBlood');
					}
					if (self.extra <= 0)
					{
						state_standard_pjibs(p, pdist);
					}
					p.pals = color(48, 52, 0, 0);
					return;
				}
				if (self.attackertype.GetClassName() == 'DukeFireext')
				{
					if (Duke.rnd(32))
					{
						if (self.sector != null) self.spawn('DukeBlood');
					}
					if (self.extra <= 0)
					{
						state_standard_pjibs(p, pdist);
					}
					p.pals = color(48, 52, 0, 0);
					return;
				}
				if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					p.pals = color(48, 52, 0, 0);
					self.PlayActorSound("EXPANDERHIT");
					return;
				}
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					p.pals = color(48, 0, 48, 0);
					setMove('PSHRINKING', 0);
					self.PlayActorSound("ACTOR_SHRINKING");
					self.cstat = 0;
					return;
				}
				if (self.attackertype.GetClassName() == 'DukeShotSpark')
				{
					p.pals = color(24, 48, 0, 0);
				}
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					p.pals = color(48, 0, 0, 48);
					if (self.extra <= 0)
					{
						self.PlayActorSound("SOMETHINGFROZE");
						self.pal = 1;
						setMove('none', 0);
						setAction('PFROZEN');
						return;
					}
				}
				if (self.attackertype.GetClassName() == 'DukeCoolExplosion1')
				{
					p.pals = color(48, 48, 0, 48);
				}
				if (self.attackertype.GetClassName() == 'DukeMeleeAttack')
				{
					p.pals = color(16, 32, 0, 0);
				}
				if (self.attackertype.GetClassName() == 'DukeFirelaser')
				{
					p.pals = color(32, 32, 0, 0);
				}
				if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
				{
					if (self.curAction.name == 'PONFIRE')
					{
						self.counter = 0;
					}
					else
					{
						self.xoffset = self.yoffset = 0;
						self.fall(p);
						if (Duke.rnd(32))
						{
							if (self.sector != null) self.spawn('DukeBlood');
						}
						if (self.extra <= 0)
						{
							state_standard_pjibs(p, pdist);
						}
						p.pals = color(48, 52, 0, 0);
						setAction('PONFIRE');
						self.counter = 0;
						return;
					}
				}
				if (self.extra <= 0)
				{
					setAction('PTHROWNBACK');
					self.GetPlayer().checkweapons();
				}
				state_random_wall_jibs(p, pdist);
				return;
			}
		}
		if (self.curAction.name == 'PFLINTCHING')
		{
			if (self.actioncounter >= 2)
			{
				setAction('PSTAND');
			}
			return;
		}
		if (self.sector.lotag == ST_2_UNDERWATER)
		{
			if (self.curAction.name == 'PTREDWATER')
			{
				if (self.checkp(p, pwalking | prunning))
				{
					setAction('PSWIMMINGGO');
				}
			}
			else if (self.checkp(p, pstanding | pwalkingback | prunningback))
			{
				setAction('PTREDWATER');
			}
			else
			{
				if (self.curAction.name == 'PSWIMMING')
				{
					if (Duke.rnd(4))
					{
						if (self.sector != null) self.spawn('DukeWaterBubble');
					}
					if (self.actioncounter >= 4)
					{
						setAction('PSWIMMINGWAIT');
					}
				}
				else if (self.curAction.name == 'PSWIMMINGWAIT')
				{
					if (self.actioncounter >= 2)
					{
						setAction('PSWIMMINGGO');
					}
				}
				else if (self.curAction.name == 'PSWIMMINGGO')
				{
					if (self.actioncounter >= 2)
					{
						setAction('PSWIMMING');
					}
				}
				else
				{
					setAction('PTREDWATER');
				}
			}
			if (Duke.rnd(4))
			{
				if (self.sector != null) self.spawn('DukeWaterBubble');
			}
			return;
		}
		else if (self.checkp(p, pjetpack))
		{
			if (self.curAction.name == 'PJPHOUVER')
			{
				if (self.actioncounter >= 4)
				{
					self.actioncounter = 0;
				}
			}
			else
			{
				setAction('PJPHOUVER');
			}
			return;
		}
		else
		{
			if (self.curAction.name == 'PTREDWATER')
			{
				setAction('PSTAND');
			}
			if (self.curAction.name == 'PSWIMMING')
			{
				setAction('PSTAND');
			}
			if (self.curAction.name == 'PSWIMMINGWAIT')
			{
				setAction('PSTAND');
			}
			if (self.curAction.name == 'PSWIMMINGGO')
			{
				setAction('PSTAND');
			}
			if (self.curAction.name == 'PJPHOUVER')
			{
				setAction('PFALLING');
			}
		}
		if (self.curAction.name == 'PFALLING')
		{
			if (self.checkp(p, ponground))
			{
				setAction('PSTAND');
			}
			else
			{
				if (self.checkp(p, pfalling))
				{
					return;
				}
				else
				{
					state_check_pstandard(p, pdist);
				}
			}
		}
		if (self.curAction.name == 'PDUCKING')
		{
			if (self.floorz - self.ceilingz < 48)
			{
				if (self.checkp(p, pwalking | pwalkingback | prunning | prunningback))
				{
					setAction('PCRAWLING');
				}
			}
			else
				if (self.checkp(p, pducking))
				{
					if (self.checkp(p, pwalking | pwalkingback | prunning | prunningback))
					{
						setAction('PCRAWLING');
					}
				}
			else
			{
				if (self.checkp(p, pstanding))
				{
					setAction('PSTAND');
				}
				else
				{
					state_check_pstandard(p, pdist);
				}
			}
		}
		else if (self.curAction.name == 'PCRAWLING')
		{
			if (self.floorz - self.ceilingz < 48)
			{
				if (self.checkp(p, pstanding))
				{
					setAction('PCRAWLING');
				}
			}
			else if (self.checkp(p, pducking))
			{
				if (self.checkp(p, pstanding))
				{
					setAction('PDUCKING');
				}
			}
			else
			{
				if (self.checkp(p, pstanding))
				{
					setAction('PSTAND');
				}
				else
				{
					state_check_pstandard(p, pdist);
				}
			}
		}
		else if (self.floorz - self.ceilingz < 48)
		{
			setAction('PDUCKING');
		}
		else
		{
			if (self.curAction.name == 'PJUMPING')
			{
				if (self.checkp(p, ponground))
				{
					setAction('PSTAND');
				}
				else
				{
					if (self.actioncounter >= 4)
					{
						if (self.checkp(p, pfalling))
						{
							setAction('PFALLING');
						}
					}
				}
			}
		}
		if (self.checkp(p, pfalling))
		{
			setAction('PFALLING');
		}
		else if (self.curAction.name == 'PSTAND')
		{
			state_check_pstandard(p, pdist);
		}
		else if (self.curAction.name == 'PAKICKING')
		{
			if (self.actioncounter >= 2)
			{
				setAction('PSTAND');
			}
			return;
		}
		else if (self.curAction.name == 'PWALK')
		{
			if (self.checkp(p, pfalling))
			{
				setAction('PFALLING');
			}
			else if (self.checkp(p, pstanding))
			{
				setAction('PSTAND');
			}
			else if (self.checkp(p, prunning))
			{
				setAction('PRUN');
			}
			else if (self.checkp(p, pwalkingback))
			{
				setAction('PWALKBACK');
			}
			else if (self.checkp(p, prunningback))
			{
				setAction('PRUNBACK');
			}
			else if (self.checkp(p, pjumping))
			{
				setAction('PJUMPING');
			}
			else if (self.checkp(p, pducking))
			{
				setAction('PDUCKING');
			}
		}
		else
			if (self.curAction.name == 'PRUN')
			{
				if (self.checkp(p, pstanding))
				{
					setAction('PSTAND');
				}
				else if (self.checkp(p, pwalking))
				{
					setAction('PWALK');
				}
				else if (self.checkp(p, pwalkingback))
				{
					setAction('PWALKBACK');
				}
				else if (self.checkp(p, prunningback))
				{
					setAction('PRUNBACK');
				}
				else if (self.checkp(p, pjumping))
				{
					setAction('PJUMPING');
				}
				else if (self.checkp(p, pducking))
				{
					setAction('PDUCKING');
				}
			}
		else if (self.curAction.name == 'PWALKBACK')
		{
			if (self.checkp(p, pstanding))
			{
				setAction('PSTAND');
			}
			else if (self.checkp(p, pwalking))
			{
				setAction('PWALK');
			}
			else if (self.checkp(p, prunning))
			{
				setAction('PRUN');
			}
			else if (self.checkp(p, prunningback))
			{
				setAction('PRUNBACK');
			}
			else if (self.checkp(p, pjumping))
			{
				setAction('PJUMPING');
			}
			else if (self.checkp(p, pducking))
			{
				setAction('PDUCKING');
			}
		}
		else if (self.curAction.name == 'PRUNBACK')
		{
			if (self.checkp(p, pstanding))
			{
				setAction('PSTAND');
			}
			else if (self.checkp(p, pwalking))
			{
				setAction('PWALK');
			} 
			else if (self.checkp(p, prunning))
			{
				setAction('PRUN');
			}
			else if (self.checkp(p, pwalkingback))
			{
				setAction('PWALKBACK');
			}
			else if (self.checkp(p, pjumping))
			{
				setAction('PJUMPING');
			}
			else if (self.checkp(p, pducking))
			{
				setAction('PDUCKING');
			}
		}
	}
}


class DukePlayerOnWater : DukeActor
{
	default
	{
		pic "PLAYERONWATER";
		+ALWAYSROTATE1;
	}
	
	override void Initialize()
	{
		if (!mapSpawned && self.ownerActor)
		{
				self.scale = self.ownerActor.scale;
				self.vel.Z = 0.5;
				if (self.sector.lotag != ST_2_UNDERWATER)
					self.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		self.ChangeStat(STAT_DUMMYPLAYER);
	}

	override void OnHit(DukeActor proj)
	{
		// propagate the hit to its owner.
		let owner = self.ownerActor;
		if (owner && self != owner) owner.OnHit(proj);
	}

}

class DukePlayerLyingDead : DukeActor
{
	default
	{
		pic "DUKELYINGDEAD";
		+HITRADIUS_FORCEEFFECT;
		action "PLYINGFRAMES", 0, 1, 0, 1, 1;
		move "DUKENOTMOVING";
		Strength 0;
		StartAction "PLYINGFRAMES";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner.isPlayer())
		{
			self.scale = owner.scale;
			self.shade = owner.shade;
			self.pal = owner.GetPlayer().palookup;
		}
		self.vel.X = 292 / 16.;
		self.vel.Z = 360 / 256.;
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.extra = 1;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'none')
		{
			if (Duke.rnd(128))
			{
				self.cstat = CSTAT_SPRITE_XFLIP;
			}
			else
			{
				self.cstat = 0;
			}
			setMove('DUKENOTMOVING', 0);
		}
		if (self.ifsquished(p))
		{
			self.PlayActorSound("SQUISHED");
			state_random_ooz(p, pdist);
			self.killit();
		}
		else if (self.counter >= 1024)
		{
			if (pdist > 4096 * maptoworld)
			{
				self.killit();
			}
		}
		else
		{
			self.extra = 0;
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					state_standard_jibs(p, pdist);
					self.killit();
				}
			}
		}
	}
	
	override bool animate(tspritetype t)
	{
		t.pos.Z += 24;
		return false;
	}
}
