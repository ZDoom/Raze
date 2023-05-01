
class DukeShark : DukeActor
{
	const SHARKSTRENGTH = 35;
	const SHARKBITESTRENGTH = -9;
	
	default
	{
		pic "SHARK";
		+INTERNAL_BADGUY;
		+DONTDIVEALIVE;
		+FLOATING;
		Strength SHARKSTRENGTH;

		action "ASHARKCRUZING", 0, 8, 5, 1, 24;
		action "ASHARKFLEE", 0, 8, 5, 1, 10;
		action "ASHARKATACK", 0, 8, 5, 1, 6;
		action "ASHARKSHRUNK", 0, 8, 5, 1, 24;
		action "ASHARKGROW", 0, 8, 5, 1, 24;
		action "ASHARKFROZEN", 0, 1, 5, 1, 24;
		move "SHARKVELS", 24;
		move "SHARKFASTVELS", 72;
		move "SHARKFLEEVELS", 40;

		StartAction "ASHARKCRUZING";
		StartMove "SHARKVELS";
		moveflags randomangle | geth;
	}
	
	override void Initialize()
	{
		// override some defaults.
		self.scale = (0.9375, 0.9375);
		self.clipdist = 10;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'ASHARKSHRUNK')
		{
			if (self.counter >= SHRUNKDONECOUNT)
			{
				setAction('ASHARKCRUZING');
			}
			else if (self.counter >= SHRUNKCOUNT)
			{
				self.actorsizeto(60 * REPEAT_SCALE, 60 * REPEAT_SCALE);
			}
			else
			{
				state_genericshrunkcode(p, pdist);
			}
			return;
		}
		else if (self.curAction.name == 'ASHARKGROW')
		{
			if (self.counter >= SHRUNKDONECOUNT)
			{
				setAction('ASHARKCRUZING');
			}
			else if (self.counter >= SHRUNKCOUNT)
			{
				self.actorsizeto(24 * REPEAT_SCALE, 24 * REPEAT_SCALE);
			}
			else
			{
				state_genericgrowcode(p, pdist, 84 * REPEAT_SCALE, 84 * REPEAT_SCALE);
			}
		}
		else if (self.curAction.name == 'ASHARKFROZEN')
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
			if (self.checkp(p, pfacing))
			{
				if (pdist < FROZENQUICKKICKDIST * maptoworld)
				{
					p.playerkick(self);
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
			if (self.counter >= THAWTIME)
			{
				setAction('ASHARKFLEE');
				self.pal = self.tempval;
				self.tempval = 0;
				return;
			}
			else
			{
				if (self.counter >= FROZENDRIPTIME)
				{
					if (self.actioncounter >= 26)
					{
						self.actioncounter = 0;
					}
				}
			}
			if (self.ifhitbyweapon() >= 0)
			{
				if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					return;
				}
				self.spriteglass(30);
				self.PlayActorSound("GLASS_BREAKING");
				self.addkill();
				self.killit();
			}
			return;
		}
		else if (self.curAction.name == 'ASHARKFLEE')
		{
			if (self.counter >= 16)
			{
				if (Duke.rnd(48))
				{
					setAction('ASHARKCRUZING');
					setMove('SHARKVELS', randomangle | geth);
				}
			}
		}
		else if (self.curAction.name == 'ASHARKCRUZING')
		{
			if (self.cansee(p))
			{
				if (self.counter >= 48)
				{
					if (Duke.rnd(2))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							setAction('ASHARKATACK');
							setMove('SHARKFASTVELS', faceplayerslow | getv);
							return;
						}
					}
				}
			}
			if (self.counter >= 32)
			{
				if (self.movflag > kHitSector)
				{
					if (Duke.rnd(128))
					{
						setMove('SHARKVELS', randomangle | geth);
					}
					else
					{
						setMove('SHARKFASTVELS', randomangle | geth);
					}
				}
			}
		}
		else
		{
			if (self.curAction.name == 'ASHARKATACK')
			{
				if (self.counter >= 4)
				{
					if (pdist < 1280 * maptoworld)
					{
						if (self.checkp(p, palive))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								self.PlayActorSound("PLAYER_GRUNT");
								p.pals = color(32, 32, 0, 0);
								p.addphealth(SHARKBITESTRENGTH, self.bBIGHEALTH);
							}
						}
						setAction('ASHARKFLEE');
						setMove('SHARKFLEEVELS', fleeenemy);
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
				else if (self.movflag > kHitSector)
				{
					if (self.counter >= 32)
					{
						setAction('ASHARKCRUZING');
						setMove('SHARKVELS', randomangle | geth);
					}
				}
				else
				{
					if (self.counter >= 48)
					{
						if (Duke.rnd(2))
						{
							setAction('ASHARKCRUZING');
							setMove('SHARKFASTVELS', randomangle | geth);
						}
					}
				}
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				if (self.attackertype.GetClassName() == 'DukeGrowSpark')
				{
					setMove('none', 0);
					self.cstat = 0;
					setAction('ASHARKGROW');
					self.PlayActorSound("ACTOR_GROWING");
					return;
				}
				else if (self.attackertype.GetClassName() == 'DukeFreezeBlast')
				{
					self.tempval = self.pal;
					self.pal = 1;
					self.extra = 0;
					setAction('ASHARKFROZEN');
					self.PlayActorSound("SOMETHINGFROZE");
				}
				else
				{
					self.PlayActorSound("SQUISHED");
					self.spawnguts('DukeJibs6', 5);
					self.addkill();
					self.killit();
				}
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeShrinkSpark')
				{
					setAction('ASHARKSHRUNK');
					self.PlayActorSound("ACTOR_SHRINKING");
					setMove('none', 0);
					return;
				}
				else
				{
					if (self.attackertype.GetClassName() == 'DukeGrowSpark')
					{
						self.PlayActorSound("EXPANDERHIT");
					}
				}
				setMove('SHARKVELS', randomangle | geth);
			}
		}
	}
}