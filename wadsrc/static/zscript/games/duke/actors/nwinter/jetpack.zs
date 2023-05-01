class NWinterJetpack : DukeItemBase
{
	const JETPACK_NEAR_EMPTY = 160;

	default
	{
		pic "JETPACK";
		+INVENTORY;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.playercheckinventory(self, GET_JETPACK, JETPACK_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								if (self.pal == 21)
								{
									p.playeraddinventory(self, GET_JETPACK, JETPACK_NEAR_EMPTY);
									p.FTA(125);
								}
								else
								{
									p.playeraddinventory(self, GET_JETPACK, JETPACK_AMOUNT);
									p.FTA(41);
								}
								if (self.attackertype.GetClassName() == 'DukeJetpack')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								self.timetosleep = SLEEPTIME;
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}

	
}


