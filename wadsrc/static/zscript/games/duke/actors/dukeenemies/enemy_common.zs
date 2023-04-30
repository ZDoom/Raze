extend class DukeActor
{

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_spawnburnedcorpse(DukePlayer p, double pdist)
	{
		if (self.sector != null) self.spawn('DukeFire2');
		if (self.sector != null) self.spawn('DukeBurnedCorpse');
		self.killit();
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_genericonfirecode(DukePlayer p, double pdist)
	{
		if (Duke.rnd(32))
		{
			self.PlayActorSound("FLESH_BURNING");
			self.extra += -6;
		}
		if (Duke.rnd(128))
		{
			if (self.sector != null) self.spawn('DukeOnFireSmoke');
		}
		if (abs(self.pos.Z - self.sector.floorz) < 32 && self.sector.lotag == ST_1_ABOVE_WATER)
		{
			self.counter = ONFIRETIME;
		}
		else
		{
			if (self.sector.lotag == ST_2_UNDERWATER)
			{
				self.counter = ONFIRETIME;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_rf(DukePlayer p, double pdist)
	{
		if (Duke.rnd(128))
		{
			self.cstat = CSTAT_SPRITE_XFLIP;
		}
		else
		{
			self.cstat = 0;
		}
	}
	
	//---------------------------------------------------------------------------
	//
	// why are these played on the player...?
	//
	//---------------------------------------------------------------------------

	void state_jib_sounds(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SWEARFREQUENCY))
		{
			if (Duke.rnd(128))
			{
				if (Duke.rnd(128))
				{
					if (Duke.rnd(128))
					{
						if (Duke.rnd(128))
						{
							if (Duke.rnd(128))
							{
								p.actor.PlayActorSound("JIBBED_ACTOR12", CHAN_AUTO, CHANF_LOCAL);
							}
							else
							{
								p.actor.PlayActorSound("JIBBED_ACTOR1", CHAN_AUTO, CHANF_LOCAL);
							}
						}
						else
						{
							if (Duke.rnd(128))
							{
								p.actor.PlayActorSound("JIBBED_ACTOR9", CHAN_AUTO, CHANF_LOCAL);
							}
							else
							{
								p.actor.PlayActorSound("JIBBED_ACTOR14", CHAN_AUTO, CHANF_LOCAL);
							}
						}
					}
					else
					{
						if (Duke.rnd(128))
						{
							p.actor.PlayActorSound("SMACKED", CHAN_AUTO, CHANF_LOCAL);
						}
						else
						{
							p.actor.PlayActorSound("JIBBED_ACTOR2", CHAN_AUTO, CHANF_LOCAL);
						}
					}
				}
				else
				{
					if (Duke.rnd(128))
					{
						p.actor.PlayActorSound("MDEVSPEECH", CHAN_AUTO, CHANF_LOCAL);
					}
					else
					{
						p.actor.PlayActorSound("JIBBED_ACTOR5", CHAN_AUTO, CHANF_LOCAL);
					}
				}
			}
			else
			{
				if (Duke.rnd(128))
				{
					if (Duke.rnd(128))
					{
						if (Duke.rnd(128))
						{
							p.actor.PlayActorSound("JIBBED_ACTOR11", CHAN_AUTO, CHANF_LOCAL);
						}
						else
						{
							p.actor.PlayActorSound("JIBBED_ACTOR13", CHAN_AUTO, CHANF_LOCAL);
						}
					}
					else
					{
						if (Duke.rnd(128))
						{
							p.actor.PlayActorSound("JIBBED_ACTOR3", CHAN_AUTO, CHANF_LOCAL);
						}
						else
						{
							p.actor.PlayActorSound("JIBBED_ACTOR8", CHAN_AUTO, CHANF_LOCAL);
						}
					}
				}
				else
				{
					if (Duke.rnd(128))
					{
						if (Duke.rnd(128))
						{
							p.actor.PlayActorSound("JIBBED_ACTOR6", CHAN_AUTO, CHANF_LOCAL);
						}
						else
						{
							p.actor.PlayActorSound("JIBBED_ACTOR4", CHAN_AUTO, CHANF_LOCAL);
						}
					}
					else
					{
						if (Duke.rnd(128))
						{
							if (Duke.rnd(128))
							{
								p.actor.PlayActorSound("JIBBED_ACTOR10", CHAN_AUTO, CHANF_LOCAL);
							}
							else
							{
								p.actor.PlayActorSound("JIBBED_ACTOR15", CHAN_AUTO, CHANF_LOCAL);
							}
						}
						else
						{
							p.actor.PlayActorSound("JIBBED_ACTOR7", CHAN_AUTO, CHANF_LOCAL);
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

	void state_standard_jibs(DukePlayer p, double pdist)
	{
		self.spawnguts('DukeJibs2', 1);
		self.spawnguts('DukeJibs3', 2);
		self.spawnguts('DukeJibs4', 3);
		self.spawnguts('DukeJibs5', 2);
		self.spawnguts('DukeJibs6', 3);
		if (Duke.rnd(6))
		{
			self.spawnguts('DukeJibs1', 1);
			if (self.sector != null) self.spawn('DukeBloodPool');
		}
		state_jib_sounds(p, pdist);
	}
	

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_random_wall_jibs(DukePlayer p, double pdist)
	{
		if (Duke.rnd(96))
		{
			self.shoot('DukeBloodSplat1');
		}
		if (Duke.rnd(96))
		{
			self.shoot('DukeBloodSplat3');
		}
		if (Duke.rnd(96))
		{
			self.shoot('DukeBloodSplat2');
		}
		if (Duke.rnd(96))
		{
			self.shoot('DukeBloodSplat4');
		}
		if (Duke.rnd(96))
		{
			self.shoot('DukeBloodSplat1');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checksquished(DukePlayer p, double pdist)
	{
		if (self.ifsquished(p))
		{
			self.addkill();
			self.PlayActorSound("SQUISHED");
			state_standard_jibs(p, pdist);
			state_random_ooz(p, pdist);
			self.killit();
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_random_ooz(DukePlayer p, double pdist)
	{
		if (Duke.rnd(128) && Duke.CheckSprite('DukeOoz2')) // add sprite check - OOZ2 was added for Atomic Edition
		{
			if (self.sector != null) self.spawn('DukeOoz2');
		}
		else
		{
			if (self.sector != null) self.spawn('DukeOoz');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_drop_ammo(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SPAWNAMMOODDS))
		{
			if (self.sector != null) self.spawn('DukeAmmo');
		}
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_drop_battery(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SPAWNAMMOODDS))
		{
			if (self.sector != null) self.spawn('DukeBatteryAmmo');
		}
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_drop_sgshells(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SPAWNAMMOODDS))
		{
			if (self.sector != null) self.spawn('DukeShotgunammo');
		}
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_drop_shotgun(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SPAWNAMMOODDS))
		{
			if (self.sector != null) self.spawn('DukeShotgunSprite');
		}
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_drop_chaingun(DukePlayer p, double pdist)
	{
		if (Duke.rnd(SPAWNAMMOODDS))
		{
			if (Duke.rnd(32))
			{
				if (self.sector != null) self.spawn('DukeChaingunSprite');
			}
			else
			{
				if (self.sector != null) self.spawn('DukeBatteryAmmo');
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_genericshrunkcode(DukePlayer p, double pdist)
	{
		if (self.counter >= 32)
		{
			if (pdist < SQUISHABLEDISTANCE * maptoworld)
			{
				p.playerstomp(self);
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		else
		{
			self.actorsizeto(MINXSTRETCH * REPEAT_SCALE, MINYSTRETCH * REPEAT_SCALE);
			if (self.sector != null) self.spawn('DukeFrameEffect');
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_genericgrowcode(DukePlayer p, double pdist, double destscalex = MAXXSTRETCH * REPEAT_SCALE, double destscaley = MAXYSTRETCH * REPEAT_SCALE)
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
			self.killit();
		}
		else
		{
			self.actorsizeto(destscalex, destscaley);
		}
	}

}
