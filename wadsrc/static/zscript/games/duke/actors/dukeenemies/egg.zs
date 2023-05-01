


class DukeEgg : DukeActor
{
	default
	{
		pic "EGG";
		Strength TOUGH;
		
		action "EGGOPEN1", 1, 1, 1, 1, 4;
		action "EGGOPEN2", 2, 1, 1, 1, 4;
		action "EGGOPEN3", 2, 1, 1, 1, 4;
		action "EGGWAIT", 0;
		action "EGGFROZEN", 1;
		action "EGGGROW", 1;
		action "EGGSHRUNK", 1;
		
	}
	
	override void Initialize()
	{
		if (ud.monsters_off == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else
		{
			self.bINTERNAL_BADGUY = true; // the egg needs this flag, but it should not run through the monster init code.
			self.clipdist = 6;
			self.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
			self.ChangeStat(STAT_ZOMBIEACTOR);
		}
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curAction.name == 'none')
		{
			if (self.counter >= 64)
			{
				if (Duke.rnd(128))
				{
					setAction('EGGWAIT');
					setMove('none', 0);
				}
				else
				{
					self.PlayActorSound("SLIM_HATCH");
					setAction('EGGOPEN1');
				}
			}
		}
		else if (self.curAction.name == 'EGGOPEN1')
		{
			if (self.actioncounter >= 4)
			{
				setAction('EGGOPEN2');
			}
		}
		else if (self.curAction.name == 'EGGOPEN2')
		{
			if (self.actioncounter >= 4)
			{
				if (self.sector != null) self.spawn('DukeGreenSlime');
				setAction('EGGOPEN3');
			}
		}
		else if (self.curAction.name == 'EGGGROW')
		{
			state_genericgrowcode(p, pdist);
		}
		else if (self.curAction.name == 'EGGSHRUNK')
		{
			state_genericshrunkcode(p, pdist);
		}
		else if (self.curAction.name == 'EGGFROZEN')
		{
			if (self.counter >= THAWTIME)
			{
				setAction('none');
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
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.PlayActorSound("SOMETHINGFROZE");
					self.tempval = self.pal;
					self.pal = 1;
					setMove('none', 0);
					setAction('EGGFROZEN');
					self.extra = 0;
					return;
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeGrowSpark')
					{
						self.cstat = 0;
						setMove('none', 0);
						self.PlayActorSound("ACTOR_GROWING");
						setAction('EGGGROW');
						return;
					}
				}
				self.PlayActorSound("SQUISHED");
				state_standard_jibs(p, pdist);
				self.killit();
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					setMove('none', 0);
					self.PlayActorSound("ACTOR_SHRINKING");
					setAction('EGGSHRUNK');
					return;
				}
			}
			if (self.attackertype.GetClassName() == 'DukeGrowSpark')
			{
				self.PlayActorSound("EXPANDERHIT");
			}
		}
		else
		{
			if (self.curAction.name == 'EGGWAIT')
			{
				if (self.counter >= 512)
				{
					if (Duke.rnd(2))
					{
						if (self.curAction.name == 'EGGSHRUNK')
						{
							return;
						}
						self.PlayActorSound("SLIM_HATCH");
						setAction('EGGOPEN1');
					}
				}
			}
		}
	}
}