
class DukeCannonball: DukeActor
{
	const CANNONBALLSTRENGTH = 400;
	default
	{
		pic "CANNONBALL";
		Strength CANNONBALLSTRENGTH;
		move "CANNONBALL1", 512, 0;
		move "CANNONBALL2", 512, 10;
		move "CANNONBALL3", 512, 20;
		move "CANNONBALL4", 512, 40;
		move "CANNONBALL5", 512, 80;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			self.Scale = (32 * REPEAT_SCALE, 32 * REPEAT_SCALE);
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			setAction('ANULLACTION');
		}
		if (self.actioncounter == 46)
		{
			setMove('CANNONBALL5', geth | getv);
		}
		else if (self.actioncounter == 44)
		{
			setMove('CANNONBALL4', geth | getv);
		}
		else if (self.actioncounter == 40)
		{
			setMove('CANNONBALL3', geth | getv);
		}
		else if (self.actioncounter == 32)
		{
			setMove('CANNONBALL2', geth | getv);
		}
		else if (self.actioncounter == 16)
		{
			setMove('CANNONBALL1', geth | getv);
		}
		if (self.movflag > kHitSector)
		{
			if (self.sector != null) self.spawn('DukeExplosion2');
			self.PlayActorSound("PIPEBOMB_EXPLODE");
			self.hitradius(4096, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
			self.killit();
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				if (self.sector != null) self.spawn('DukeExplosion2');
				self.hitradius(4096, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
				self.killit();
			}
			else
			{
				self.spawndebris(DukeScrap.Scrap1, 3);
			}
		}
	}
	
}

class DukeCannonballs : DukeActor // (1818)
{
	const CANNONBALLSSTRENGTH = 10;
	default
	{
		pic "CANNONBALLS";
		Strength CANNONBALLSSTRENGTH;
		move "CANNONBALLSVEL";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			setAction('ANULLACTION');
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				if (self.sector != null) self.spawn('DukeExplosion2');
				self.hitradius(4096, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
				self.killit();
			}
			else
			{
				self.spawndebris(DukeScrap.Scrap1, 3);
			}
		}
	}
}

class DukeCannon : DukeActor // (1810)
{
	const CANNONSTRENGTH = 400;
	default
	{
		+BADGUY
		Strength CANNONSTRENGTH;
		pic "CANNON";
		action "ACANNONWAIT", 0, 1, 7, 1, 1;
		action "ACANNONSHOOTING", 0, 1, 7, 1, 1;
		move "CANNONSTOP";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('ACANNONWAIT');
		}
		else if (self.curAction.name == 'ACANNONSHOOTING')
		{
			if (self.sector != null) self.spawn('DukeCannonball');
			setAction('ACANNONWAIT');
		}
		else
		{
			if (self.curAction.name == 'ACANNONWAIT')
			{
				if (self.actioncounter >= 64)
				{
					if (Duke.rnd(128))
					{
						setAction('ACANNONSHOOTING');
					}
					else
					{
						self.actioncounter = 0;
					}
				}
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.addkill();
				self.hitradius(4096, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
				if (self.sector != null) self.spawn('DukeExplosion2');
				self.killit();
			}
			else
			{
				self.spawndebris(DukeScrap.Scrap1, 3);
			}
		}
		if (pdist < 1024 * maptoworld)
		{
			if (p.PlayerInput(Duke.SB_OPEN))
			{
				if (self.checkp(p, pfacing))
				{
					if (self.ifcanshoottarget(p, pdist))
					{
						if (self.sector != null) self.spawn('DukeCannonball');
					}
				}
				else
				{
					return;
				}
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
	}
	
}


