class DukeRubberCan : DukeActor
{
	default
	{
		pic "RUBBERCAN";
		Strength WEAK;
		+FORCERUNCON;
		+CHECKSLEEP;
		+MOVEFTA_MAKESTANDABLE;
		
		move "RESPAWN_ACTOR_FLAG";
		action "RUBCANDENT", 1, 1, 1, 1, 1;
		
	}

	override void Initialize()
	{
		if (!self.mapSpawned)
			self.scale = (0.5, 0.5);
		self.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
		self.clipdist = 18;
		self.ChangeStat(STAT_ZOMBIEACTOR);
		self.extra = 0;
	}
	
	void state_rats(DukePlayer p, double pdist)
	{
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
		if (Duke.rnd(128))
		{
			self.spawn('DukeRat');
		}
	}
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'RUBCANDENT')
		{
			if (self.actioncounter >= 16)
			{
				self.extra = 0;
				setAction('none');
				return;
			}
		}
		else
		{
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
				{
					if (self.sector != null) 
					{
						state_rats(p, pdist);
						if (Duke.rnd(48))
						{
							self.spawn('DukeBurning');
						}
						self.spawndebris(DukeScrap.Scrap3, 12);
					}
					self.killit();
				}
				else
				{
					setAction('RUBCANDENT');
				}
			}
		}
	}
}


