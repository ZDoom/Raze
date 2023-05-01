class NWinterXmasPresent : DukeActor
{
	default
	{
		pic "XMASPRESENT";
		action "REDPRESENT", 4, 1, 1, 1, 1;
	}
	void state_redpresent(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.checkp(p, pshrunk))
		{
		}
		else
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							self.PlayActorSound("RIP_PAPER");
							if (Duke.rnd(128))
							{
								if (self.sector != null) self.spawn('DukeShield');
							}
							else
							{
								if (self.sector != null) self.spawn('DukeSixpak');
							}
							self.killit();
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
	

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			self.Scale = (20 * REPEAT_SCALE, 20 * REPEAT_SCALE);
			setAction('REDPRESENT');
		}
		state_redpresent(p, pdist);
	}

}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

class NWinterXmasPresent2 : DukeActor
{
	default
	{
		pic "XMASPRESENT";
		action "GREENPRESENT", 0, 1, 1, 1, 1;
	}

	void state_greenpresent(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.checkp(p, pshrunk))
		{
		}
		else
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							self.PlayActorSound("RIP_PAPER");
							if (Duke.rnd(128))
							{
								if (self.sector != null) self.spawn('DukeChaingunSprite');
							}
							else
							{
								if (self.sector != null) self.spawn('DukeShotgunSprite');
							}
							self.killit();
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			self.Scale = (20 * REPEAT_SCALE, 20 * REPEAT_SCALE);
			setAction('GREENPRESENT');
		}
		state_greenpresent(p, pdist);
	}

}
