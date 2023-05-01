
class DukeRotateGun : DukeActor
{
	const ROTTURRETSTRENGTH = 40;
	
	default
	{
		pic "ROTATEGUN";
		Strength ROTTURRETSTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+NORADIUSPUSH;
		+SHOOTCENTERED;
		+NOVERTICALMOVE;
		+MOVE_NOPLAYERINTERACT;
		+NOHITJIBS;
		+NOSHOTGUNBLOOD;
		
		sparkoffset -8;
		aimoffset 32;
		shootzoffset 0;
		
		action "ASATNSPIN", 0, 5, 1, 1, 4;
		action "ASATSHOOTING", -10, 3, 5, 1, 40;
		action "ASATWAIT", 0, 1, 5, 1, 1;
		move "TURRVEL";
		
	}
	
	override void Initialize()
	{
		self.vel.Z = 0;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			setAction('ASATSHOOTING');
			setMove('TURRVEL', faceplayer);
		}
		else if (self.curAction.name == 'ASATNSPIN')
		{
			if (self.actioncounter >= 32)
			{
				setAction('ASATWAIT');
				setMove('TURRVEL', faceplayer);
			}
		}
		else if (self.curAction.name == 'ASATSHOOTING')
		{
			if (self.actioncounter >= 12)
			{
				if (Duke.rnd(32))
				{
					setAction('ASATWAIT');
					setMove('none', 0);
				}
			}
			if (self.counter >= 32)
			{
				self.counter = 0;
			}
			else if (self.counter >= 16)
			{
				if (self.counter >= 17)
				{
				}
				else
				{
					self.PlayActorSound("PRED_ATTACK");
					self.shoot('DukeFirelaser');
				}
			}
			else
			{
				if (self.counter >= 4)
				{
					if (self.counter >= 5)
					{
					}
					else
					{
						if (self.cansee(p))
						{
							if (self.ifcanshoottarget(p, pdist))
							{
								self.PlayActorSound("PRED_ATTACK");
								self.shoot('DukeFirelaser');
							}
						}
					}
				}
			}
		}
		else
		{
			if (self.curAction.name == 'ASATWAIT')
			{
				if (self.actioncounter >= 64)
				{
					if (Duke.rnd(32))
					{
						if (self.checkp(p, palive))
						{
							if (self.cansee(p))
							{
								setAction('ASATSHOOTING');
								setMove('TURRVEL', faceplayer);
							}
						}
					}
				}
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.addkill();
				self.PlayActorSound("LASERTRIP_EXPLODE");
				self.spawndebris(DukeScrap.Scrap1, 15);
				if (self.sector != null) self.spawn('DukeExplosion2');
				self.killit();
			}
			else
			{
				if (self.attackertype.GetClassName() != 'DukeFlamethrowerFlame')
				{
					setAction('ASATNSPIN');
					self.spawndebris(DukeScrap.Scrap1, 4);
				}
			}
			setMove('none', 0);
		}
	}
}