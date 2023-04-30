
class DukeLizTrooper : DukeActor
{
	const TROOPSTRENGTH = 30;
	
	default
	{
		pic "LIZTROOP";
		Strength TROOPSTRENGTH;
		precacheclass "DukeHeadJib1", "DukeArmJib1", "DukeLegJib1";
		StartAction "ATROOPSTAND";
		
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+TRANSFERPALTOJIBS;
		+DONTENTERWATERONGROUND;
		
		action "ATROOPSTAND", 0, 1, 5, 1, 1;
		action "ATROOPGROW", 0, 1, 5, 1, 1;
		action "ATROOPSTAYSTAND", -2, 1, 5, 1, 1;
		action "ATROOPWALKING", 0, 4, 5, 1, 12;
		action "ATROOPWALKINGBACK", 15, 4, 5, -1, 12;
		action "ATROOPRUNNING", 0, 4, 5, 1, 8;
		action "ATROOPSHOOT", 35, 1, 5, 1, 30;
		action "ATROOPJETPACK", 40, 1, 5, 1, 1;
		action "ATROOPJETPACKILL", 40, 2, 5, 1, 50;
		action "ATROOPFLINTCH", 50, 1, 1, 1, 6;
		action "ATROOPFLINTCHFIRE", 50, 1, 1, 1, 6;
		action "ATROOPDYING", 50, 5, 1, 1, 16;
		action "ATROOPDEAD", 54;
		action "ATROOPPLAYDEAD", 54;
		action "ATROOPSUFFERDEAD", 58, 2, 1, -4, 24;
		action "ATROOPSUFFERING", 59, 2, 1, 1, 21;
		action "ATROOPDUCK", 64, 1, 5, 1, 3;
		action "ATROOPDUCKSHOOT", 64, 2, 5, 1, 25;
		action "ATROOPABOUTHIDE", 74, 1, 1, 1, 25;
		action "ATROOPHIDE", 79, 1, 1, 1, 25;
		action "ATROOPREAPPEAR", 74, 1, 1, 1, 25;
		action "ATROOPFROZEN", 0, 1, 5;
		move "TROOPWALKVELS", 72;
		move "TROOPWALKVELSBACK", -72;
		move "TROOPJETPACKVELS", 64, -84;
		move "TROOPJETPACKILLVELS", 192, -38;
		move "TROOPRUNVELS", 108;
		move "TROOPSTOPPED";
		move "DONTGETUP";
		move "SHRUNKVELS", 32;
		ai "AITROOPSEEKENEMY", "ATROOPWALKING", "TROOPWALKVELS", seekplayer;
		ai "AITROOPSEEKPLAYER", "ATROOPWALKING", "TROOPWALKVELS", seekplayer;
		ai "AITROOPFLEEING", "ATROOPWALKING", "TROOPWALKVELS", fleeenemy;
		ai "AITROOPFLEEINGBACK", "ATROOPWALKINGBACK", "TROOPWALKVELSBACK", faceplayer;
		ai "AITROOPDODGE", "ATROOPWALKING", "TROOPRUNVELS", dodgebullet;
		ai "AITROOPSHOOTING", "ATROOPSHOOT", "TROOPSTOPPED", faceplayer;
		ai "AITROOPDUCKING", "ATROOPDUCK", "TROOPSTOPPED", faceplayer;
		ai "AITROOPJETPACK", "ATROOPJETPACK", "TROOPJETPACKVELS", seekplayer;
		ai "AITROOPSHRUNK", "ATROOPWALKING", "SHRUNKVELS", fleeenemy;
		ai "AITROOPHIDE", "ATROOPABOUTHIDE", "TROOPSTOPPED", faceplayer;
		ai "AITROOPGROW", "ATROOPGROW", "DONTGETUP", faceplayerslow;
		ai "AITROOPONFIRE", "ATROOPWALKING", "TROOPWALKVELS", fleeenemy;
		
	}
	
	override void Initialize()
	{
		if (self.pal == 0 || self.pal == 2) self.pal = 22;
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("PRED_RECOG");
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troop_body_jibs(DukePlayer p, double pdist)
	{
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeHeadJib1', 1);
		}
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeLegJib1', 2);
		}
		if (Duke.rnd(64))
		{
			self.spawnguts('DukeArmJib1', 1);
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

	void state_trooponfirestate(DukePlayer p, double pdist)
	{
		if (self.counter >= ONFIRETIME)
		{
			self.pal = self.tempval;
			self.tempval = 0;
			SetAI('AITROOPSEEKENEMY');
		}
		else
		{
			state_genericonfirecode(p, pdist);
			if (Duke.rnd(FIREPAINFREQ))
			{
				self.PlayActorSound("PRED_PAIN");
			}
		}
		if (self.extra <= 0) // ifstrength
		{
			SetAction('ATROOPDYING');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troophidestate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ATROOPREAPPEAR')
		{
			if (self.actioncounter >= 2)
			{
				self.PlayActorSound("TELEPORTER");
				SetAI('AITROOPSHOOTING');
				self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			}
			else
			{
				self.actorsizeto(41 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(41 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(41 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(41 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				if (self.sector != null) self.spawn('DukeFrameEffect');
			}
		}
		else if (self.curAction.name == 'ATROOPWALKING')
		{
			if (pdist < 2448 * maptoworld)
			{
				if (pdist > 1024 * maptoworld)
				{
					if (self.pos.Z - self.ceilingz < 48)
					{
						return;
					}
					if (self.checkp(p, pfacing))
					{
						return;
					}
					if (self.floorz - self.ceilingz < 64)
					{
					}
					else
					{
						if (self.isAwayFromWall(6.75))
						{
							if (self.sector != null) self.spawn('DukeTransporterStar');
							SetAction('ATROOPREAPPEAR');
							SetMove('none', 0);
							return;
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		else if (self.curAction.name == 'ATROOPHIDE')
		{
			if (self.actioncounter >= 2)
			{
				if (self.sector != null) self.spawn('DukeTransporterStar');
				self.PlayActorSound("TELEPORTER");
				SetAction('ATROOPWALKING');
				SetMove('TROOPWALKVELS', faceplayer);
				self.cstat = CSTAT_SPRITE_INVISIBLE;
			}
			else
			{
				self.actorsizeto(4 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(4 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(4 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				self.actorsizeto(4 * REPEAT_SCALE, 40 * REPEAT_SCALE);
				if (self.sector != null) self.spawn('DukeFrameEffect');
			}
		}
		else
		{
			if (self.curAction.name == 'ATROOPABOUTHIDE')
			{
				if (self.actioncounter >= 2)
				{
					SetAction('ATROOPHIDE');
					self.cstat = 0;
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troopgunnashoot(DukePlayer p, double pdist)
	{
		if (self.checkp(p, palive))
		{
			if (pdist < 1024 * maptoworld)
			{
				SetAI('AITROOPSHOOTING');
			}
			else if (self.actorstayput == null)
			{
				if (self.actioncounter >= 12)
				{
					if (Duke.rnd(16))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							if (self.pal == 21)
							{
								if (Duke.rnd(4))
								{
									if (pdist > 4096 * maptoworld)
									{
										SetAI('AITROOPHIDE');
									}
								}
							}
							else
							{
								if (pdist < 1100 * maptoworld)
								{
									SetAI('AITROOPFLEEING');
								}
								else
								{
									if (pdist < 4096 * maptoworld)
									{
										if (self.cansee(p))
										{
											if (self.ifcanshoottarget(p, pdist))
											{
												SetAI('AITROOPSHOOTING');
											}
										}
									}
									else
									{
										SetMove('TROOPRUNVELS', seekplayer);
										SetAction('ATROOPRUNNING');
									}
								}
							}
						}
					}
				}
			}
			else
			{
				if (self.counter >= 26)
				{
					if (Duke.rnd(32))
					{
						SetAI('AITROOPSHOOTING');
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

	void state_troopseekstate(DukePlayer p, double pdist)
	{
		state_troopgunnashoot(p, pdist);
		if (self.sector.lotag == ST_2_UNDERWATER)
		{
			SetAI('AITROOPJETPACK');
			return;
		}
		if (self.cansee(p))
		{
			if (self.curMove.name == 'TROOPRUNVELS')
			{
				if (pdist < 1596 * maptoworld)
				{
					SetAI('AITROOPDUCKING');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			if (self.checkp(p, phigher))
			{
				if (self.pos.Z - self.ceilingz < 128)
				{
				}
				else
				{
					if (self.actorstayput == null)
					{
						SetAI('AITROOPJETPACK');
					}
				}
				return;
			}
			else
			{
				if (Duke.rnd(2))
				{
					if (self.pal == 21)
					{
						if (pdist > 1596 * maptoworld)
						{
							SetAI('AITROOPHIDE');
							return;
						}
						if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
					}
					if (self.dodge() == 1)
					{
						if (Duke.rnd(128))
						{
							SetAI('AITROOPDODGE');
						}
						else
						{
							SetAI('AITROOPDUCKING');
						}
						return;
					}
				}
			}
		}
		if (self.movflag > kHitSector)
		{
			if (Duke.rnd(32))
			{
				self.actoroperate();
			}
			else
			{
				if (self.counter >= 32)
				{
					if (self.checkp(p, palive))
					{
						if (self.cansee(p))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								SetAI('AITROOPSHOOTING');
							}
						}
					}
				}
			}
		}
		if (Duke.rnd(1))
		{
			if (Duke.rnd(128))
			{
				self.PlayActorSound("PRED_ROAM", CHAN_AUTO, CHANF_SINGULAR);
			}
			else
			{
				self.PlayActorSound("PRED_ROAM2", CHAN_AUTO, CHANF_SINGULAR);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troopduckstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ATROOPDUCK')
		{
			if (self.actioncounter >= 8)
			{
				if (self.checkp(p, palive))
				{
					if (Duke.rnd(128))
					{
						SetAction('ATROOPDUCKSHOOT');
					}
				}
				else if (self.curMove.name == 'DONTGETUP')
				{
					return;
				}
				else
				{
					SetAI('AITROOPSEEKPLAYER');
				}
			}
		}
		else
		{
			if (self.curAction.name == 'ATROOPDUCKSHOOT')
			{
				if (self.counter >= 64)
				{
					if (self.curMove.name == 'DONTGETUP')
					{
						self.counter = 0;
					}
					else if (pdist < 1100 * maptoworld)
					{
						SetAI('AITROOPFLEEING');
					}
					else
					{
						SetAI('AITROOPSEEKPLAYER');
					}
				}
				else
				{
					if (self.actioncounter >= 2)
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							self.PlayActorSound("PRED_ATTACK");
							self.actioncounter = 0;
							self.shoot('DukeFirelaser');
						}
						else
						{
							SetAI('AITROOPSEEKPLAYER');
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

	void state_troopshootstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 2)
		{
			if (self.ifcanshoottarget(p, pdist))
			{
				self.shoot('DukeFirelaser');
				self.PlayActorSound("PRED_ATTACK");
				self.actioncounter = 0;
				if (Duke.rnd(128))
				{
					SetAI('AITROOPSEEKPLAYER');
				}
				if (self.counter >= 24)
				{
					if (Duke.rnd(96))
					{
						if (pdist > 2048 * maptoworld)
						{
							SetAI('AITROOPSEEKPLAYER');
						}
					}
					else
					{
						if (pdist > 1596 * maptoworld)
						{
							SetAI('AITROOPFLEEING');
						}
						else
						{
							SetAI('AITROOPFLEEINGBACK');
						}
					}
				}
			}
			else
			{
				SetAI('AITROOPSEEKPLAYER');
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troopfleestate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 7)
		{
			if (pdist > 3084 * maptoworld)
			{
				SetAI('AITROOPSEEKPLAYER');
				return;
			}
			else
			{
				if (Duke.rnd(32))
				{
					if (self.checkp(p, palive))
					{
						if (self.cansee(p))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								if (Duke.rnd(128))
								{
									SetAI('AITROOPDUCKING');
								}
								else
								{
									SetAI('AITROOPSHOOTING');
								}
								return;
							}
						}
					}
				}
			}
		}
		if (self.movflag > kHitSector)
		{
			if (Duke.rnd(32))
			{
				self.actoroperate();
			}
			else
			{
				if (self.counter >= 32)
				{
					if (self.checkp(p, palive))
					{
						if (self.cansee(p))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								if (Duke.rnd(128))
								{
									SetAI('AITROOPSHOOTING');
								}
								else
								{
									SetAI('AITROOPDUCKING');
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

	void state_troopdying(DukePlayer p, double pdist)
	{
		if (self.floorz - self.pos.Z < 32)
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
					self.cstat = 0;
					if (self.floorz - self.pos.Z < 8)
					{
						self.PlayActorSound("THUD");
					}
					if (Duke.rnd(64))
					{
						if (self.sector != null) self.spawn('DukeBloodPool');
					}
					state_rf(p, pdist);
					self.extra = 0;
				}
				SetMove('TROOPSTOPPED', 0);
				SetAction('ATROOPDEAD');
			}
		}
		else
		{
			state_rf(p, pdist);
			SetMove('none', 0);
			SetAction('ATROOPDYING');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checktroophit(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ATROOPSUFFERING')
		{
			Duke.StopSound("LIZARD_BEG");
			self.PlayActorSound("PRED_DYING");
			self.cstat = 0;
			self.extra = 0;
			SetAction('ATROOPSUFFERDEAD');
			return;
		}
		if (self.extra < 0)
		{
			if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
			{
				self.PlayActorSound("SOMETHINGFROZE");
				self.tempval = self.pal;
				self.pal = 1;
				SetMove('none', 0);
				SetAction('ATROOPFROZEN');
				self.extra = 0;
				return;
			}
			state_drop_ammo(p, pdist);
			state_random_wall_jibs(p, pdist);
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.cstat = 0;
				self.PlayActorSound("ACTOR_GROWING");
				SetAI('AITROOPGROW');
				return;
			}
			self.addkill();
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.PlayActorSound("SQUISHED");
				state_troop_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				self.PlayActorSound("SQUISHED");
				state_troop_body_jibs(p, pdist);
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				state_spawnburnedcorpse(p, pdist);
			}
			else
			{
				self.PlayActorSound("PRED_DYING");
				if (Duke.rnd(32))
				{
					if (self.floorz - self.pos.Z < 32)
					{
						self.PlayActorSound("LIZARD_BEG");
						if (self.sector != null) self.spawn('DukeBloodPool');
						self.extra = 0;
						SetMove('none', 0);
						SetAction('ATROOPSUFFERING');
						return;
					}
				}
				SetAction('ATROOPDYING');
			}
		}
		else
		{
			if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
			}
			else
			{
				state_random_wall_jibs(p, pdist);
				self.PlayActorSound("PRED_PAIN");
			}
			if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
			{
				if (self.pal == 2)
				{
					self.pal = self.tempval;
					self.tempval = 0;
				}
				self.PlayActorSound("ACTOR_SHRINKING");
				SetAI('AITROOPSHRUNK');
			}
			else if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				if (self.pal != 2)
				{
					self.tempval = self.pal;
					self.pal = 2;
					SetAI('AITROOPONFIRE');
					self.counter = 0;
					state_trooponfirestate(p, pdist);
				}
			}
			else if (self.curAI != 'AITROOPONFIRE')
			{
				if (self.floorz - self.pos.Z < 32)
				{
					if (Duke.rnd(96))
					{
						SetAction('ATROOPFLINTCH');
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

	void state_troopjetpackstate(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ATROOPJETPACKILL')
		{
			if (self.cansee(p))
			{
				if (self.actioncounter >= 2)
				{
					self.actioncounter = 0;
					self.PlayActorSound("PRED_ATTACK");
					self.shoot('DukeFirelaser');
				}
			}
			if (self.checkp(p, phigher))
			{
				SetAI('AITROOPJETPACK');
			}
			else if (self.sector.lotag == ST_2_UNDERWATER)
			{
				SetAI('AITROOPJETPACK');
			}
			else
			{
				if (self.counter >= 26)
				{
					if (self.floorz - self.pos.Z < 32)
					{
						SetAI('AITROOPSEEKPLAYER');
					}
				}
			}
		}
		else
		{
			if (self.counter >= 48)
			{
				if (self.cansee(p))
				{
					SetAction('ATROOPJETPACKILL');
					SetMove('TROOPJETPACKILLVELS', seekplayer);
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troopsufferingstate(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 2)
		{
			if (Duke.rnd(16))
			{
				if (self.sector != null) self.spawn('DukeWaterDrip');
			}
			if (self.actioncounter >= 14)
			{
				Duke.StopSound("LIZARD_BEG");
				self.cstat = 0;
				self.extra = 0;
				SetAction('ATROOPSUFFERDEAD');
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_troopshrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= SHRUNKDONECOUNT)
		{
			SetAI('AITROOPSEEKENEMY');
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

	void state_troopcode(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.sector.lotag == ST_2_UNDERWATER)
		{
			if (Duke.rnd(1))
			{
				if (self.sector != null) self.spawn('DukeWaterBubble');
			}
		}
		if (self.curAction.name == 'ATROOPSTAND')
		{
			if (Duke.rnd(192))
			{
				SetAI('AITROOPSHOOTING');
			}
			else
			{
				SetAI('AITROOPSEEKPLAYER');
			}
		}
		else if (self.curAction.name == 'ATROOPFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				SetAI('AITROOPSEEKENEMY');
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
		else if (self.curAction.name == 'ATROOPPLAYDEAD')
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					self.PlayActorSound("SQUISHED");
					state_troop_body_jibs(p, pdist);
					state_standard_jibs(p, pdist);
					self.killit();
				}
				return;
			}
			else
			{
				state_checksquished(p, pdist);
			}
			if (self.counter >= PLAYDEADTIME)
			{
				self.subkill();
				self.PlayActorSound("PRED_ROAM", CHAN_AUTO, CHANF_SINGULAR);
				self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
				self.extra = 1;
				SetAI('AITROOPSHOOTING');
			}
			else
			{
				if (self.checkp(p, pfacing))
				{
					self.counter = 0;
				}
			}
			return;
		}
		else if (self.curAction.name == 'ATROOPDEAD')
		{
			self.extra = 0;
			if (ud.respawn_monsters)
			{
				if (self.counter >= RESPAWNACTORTIME)
				{
					if (self.sector != null) self.spawn('DukeTransporterStar');
					self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
					self.extra = TROOPSTRENGTH;
					SetAI('AITROOPSEEKENEMY');
				}
			}
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					self.PlayActorSound("SQUISHED");
					state_troop_body_jibs(p, pdist);
					state_standard_jibs(p, pdist);
					self.killit();
				}
				return;
			}
			else
			{
				state_checksquished(p, pdist);
			}
			return;
		}
		else if (self.curAction.name == 'ATROOPSUFFERDEAD')
		{
			if (self.actioncounter >= 2)
			{
				if (Duke.rnd(64))
				{
					self.counter = 0;
					SetAction('ATROOPPLAYDEAD');
				}
				else
				{
					self.PlayActorSound("PRED_DYING", CHAN_AUTO, CHANF_SINGULAR);
					SetAction('ATROOPDEAD');
				}
			}
		}
		else if (self.curAction.name == 'ATROOPDYING')
		{
			state_troopdying(p, pdist);
			return;
		}
		else if (self.curAction.name == 'ATROOPSUFFERING')
		{
			state_troopsufferingstate(p, pdist);
			if (self.ifhitbyweapon() >= 0)
			{
				state_checktroophit(p, pdist);
			}
			return;
		}
		else if (self.curAction.name == 'ATROOPFLINTCH')
		{
			if (self.actioncounter >= 4)
			{
				SetAI('AITROOPSEEKENEMY');
			}
		}
		else if (self.curAI == 'AITROOPSEEKPLAYER')
		{
			state_troopseekstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPJETPACK')
		{
			state_troopjetpackstate(p, pdist);
			if (self.sector.lotag == ST_2_UNDERWATER)
			{
			}
			else
			{
				self.PlayActorSound("PLAYER_JETPACK_IDLE", CHAN_AUTO, CHANF_SINGULAR);
			}
		}
		else if (self.curAI == 'AITROOPSEEKENEMY')
		{
			state_troopseekstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPSHOOTING')
		{
			state_troopshootstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPFLEEING')
		{
			state_troopfleestate(p, pdist);
		}
		else if (self.curAI == 'AITROOPFLEEINGBACK')
		{
			state_troopfleestate(p, pdist);
		}
		else if (self.curAI == 'AITROOPDODGE')
		{
			state_troopseekstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPDUCKING')
		{
			state_troopduckstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPSHRUNK')
		{
			state_troopshrunkstate(p, pdist);
		}
		else if (self.curAI == 'AITROOPGROW')
		{
			state_genericgrowcode(p, pdist);
		}
		else if (self.curAI == 'AITROOPHIDE')
		{
			state_troophidestate(p, pdist);
			return;
		}
		else if (self.curAI == 'AITROOPONFIRE')
		{
			state_trooponfirestate(p, pdist);
		}

		if (self.ifhitbyweapon() >= 0)
		{
			state_checktroophit(p, pdist);
		}
		else
		{
			state_checksquished(p, pdist);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checktrooppalette(DukePlayer p, double pdist)
	{
		if (self.curAI == 'none')
		{
			if (self.pal == 21)
			{
				self.extra += TROOPSTRENGTH;
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
		state_checktrooppalette(p, pdist);
		state_troopcode(p, pdist);
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperToilet : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPONTOILET";
		StartAction "none";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.counter >= 24)
		{
			self.PlayActorSound("FLUSH_TOILET");
			self.actoroperate();
			SetAI('AITROOPSEEKPLAYER');
			self.ChangeType('DukeLizTrooper');
		}
		else if (self.counter < 2)
		{
			state_checktrooppalette(p, pdist);
		}
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperSitting : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJUSTSIT";
		StartAction "none";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.counter >= 30)
		{
			self.actoroperate();
			SetAI('AITROOPSEEKPLAYER');
			self.ChangeType('DukeLizTrooper');
		}
		else if (self.counter < 2)
		{
			state_checktrooppalette(p, pdist);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperShoot : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSHOOT";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checktrooppalette(p, pdist);
		SetAI('AITROOPSHOOTING');
		self.ChangeType('DukeLizTrooper');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperJetpack : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJETPACK";
		startaction "none";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checktrooppalette(p, pdist);
		SetAI('AITROOPJETPACK');
		self.ChangeType('DukeLizTrooper');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperDucking : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPDUCKING";
		startaction "none";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checktrooppalette(p, pdist);
		SetAI('AITROOPDUCKING');
		self.ChangeType('DukeLizTrooper');
		if (self.floorz - self.ceilingz < 48)
		{
			SetMove('DONTGETUP', 0);
		}
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperRunning : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPRUNNING";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		state_checktrooppalette(p, pdist);
		SetAI('AITROOPSEEKPLAYER');
		self.ChangeType('DukeLizTrooper');
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperStayput : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSTAYPUT";
		StartAction "ATROOPSTAYSTAND";
	}
	
	override void PlayFTASound(int mode)
	{
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		state_checktrooppalette(p, pdist);
		SetAI('AITROOPSEEKPLAYER');
		self.ChangeType('DukeLizTrooper');
	}
	
}
