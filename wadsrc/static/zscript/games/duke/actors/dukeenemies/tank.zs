class DukeTank : DukeActor
{
	const TANKSTRENGTH = 500;
	meta class<DukeActor> spawntype;
	property spawntype: spawntype;

 	default
	{
		pic "TANK";
		Strength TANKSTRENGTH;
		+BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+NORADIUSPUSH;

		action "ATANKSPIN", 0, 1, 7, 1, 4;
		action "ATANKSHOOTING", 7, 2, 7, 1, 10;
		action "ATANKWAIT", 0, 1, 7, 1, 1;
		action "ATANKDESTRUCT", 0, 1, 7, 1, 1;
		action "ATANKDEAD", 0, 1, 7, 1, 1;
		move "TANKFORWARD", 100;
		move "TANKSTOP";
		DukeTank.SpawnType "DukePigCop";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curAction.name == 'none')
		{
			self.Scale = (60 * REPEAT_SCALE, 60 * REPEAT_SCALE);
			setAction('ATANKWAIT');
			self.cstat = CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.clipdist += 100 * 0.25;
		}
		else if (self.curAction.name == 'ATANKSPIN')
		{
			self.PlayActorSound("TANK_ROAM", CHAN_AUTO, CHANF_SINGULAR);
			if (self.actioncounter >= 20)
			{
				if (Duke.rnd(16))
				{
					if (self.cansee(p))
					{
						if (self.ifcanshoottarget(p, pdist))
						{
							setMove('TANKSTOP', geth);
							setAction('ATANKSHOOTING');
							Duke.StopSound("TANK_ROAM");
						}
					}
				}
			}
			if (Duke.rnd(16))
			{
				setMove('TANKFORWARD', seekplayer);
			}
		}
		else if (self.curAction.name == 'ATANKSHOOTING')
		{
			if (self.actioncounter >= 22)
			{
				if (pdist > 8192 * maptoworld)
				{
					self.PlayActorSound("BOS1_ATTACK2");
					self.shoot('DukeMortar');
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				self.counter = 0;
				setMove('none', 0);
				setAction('ATANKWAIT');
			}
			else
			{
				if (self.actioncounter >= 2)
				{
					if (self.cansee(p))
					{
						if (pdist < 16384 * maptoworld)
						{
							if (Duke.rnd(128))
							{
								self.PlayActorSound("PISTOL_FIRE");
								self.shoot('DukeShotSpark');
							}
						}
						else
						{
							if (Duke.rnd(128))
							{
								self.PlayActorSound("PRED_ATTACK");
								self.shoot('DukeFirelaser');
							}
						}
					}
					else
					{
						setMove('TANKFORWARD', seekplayer);
						setAction('ATANKSPIN');
					}
				}
			}
			if (Duke.rnd(16))
			{
				Duke.StopSound("TANK_ROAM");
				setMove('TANKSTOP', faceplayerslow);
			}
		}
		else if (self.curAction.name == 'ATANKWAIT')
		{
			if (self.actioncounter >= 32)
			{
				setMove('TANKFORWARD', seekplayer);
				setAction('ATANKSPIN');
			}
		}
		else if (self.curAction.name == 'ATANKDESTRUCT')
		{
			if (self.actioncounter >= 64)
			{
				setAction('ATANKDEAD');
			}
			else if (self.actioncounter >= 56)
			{
				self.PlayActorSound("LASERTRIP_ARMING");
			}
			else if (self.actioncounter >= 48)
			{
				self.PlayActorSound("LASERTRIP_ARMING");
			}
			else if (self.actioncounter >= 32)
			{
				self.PlayActorSound("LASERTRIP_ARMING");
			}
			else if (self.actioncounter >= 16)
			{
				self.PlayActorSound("LASERTRIP_ARMING");
			}
			return;
		}
		else
		{
			if (self.curAction.name == 'ATANKDEAD')
			{
				self.addkill();
				self.hitradius(6144, TOUGH, TOUGH, TOUGH, TOUGH);
				self.PlayActorSound("LASERTRIP_EXPLODE");
				self.spawndebris(DukeScrap.Scrap1, 15);
				if (self.sector != null) self.spawn('DukeExplosion2');
				if (Duke.rnd(128))
				{
					if (self.sector != null) self.spawn(self.spawntype);
				}
				self.killit();
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				setAction('ATANKDEAD');
			}
			else
			{
				self.spawndebris(DukeScrap.Scrap1, 1);
				if (self.curAction.name == 'ATANKSHOOTING')
				{
					return;
				}
				if (Duke.rnd(192))
				{
					setMove('TANKSTOP', geth);
					setAction('ATANKSHOOTING');
					Duke.StopSound("TANK_ROAM");
				}
			}
		}
		if (pdist < 1280 * maptoworld)
		{
			if (p.PlayerInput(Duke.SB_OPEN))
			{
				if (self.checkp(p, pfacing))
				{
					if (absangle(p.actor.angle, self.angle) < 90)
					{
						setAction('ATANKDESTRUCT');
					}
				}
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
	}
}
