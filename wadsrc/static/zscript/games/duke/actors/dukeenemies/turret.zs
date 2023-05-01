class DukeTurret : DukeActor
{
	const TURRETSTRENGTH = 30;

	default
	{
		pic "ORGANTIC";
		Strength TURRETSTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOVERTICALMOVE;
		+NOHITJIBS;
		+NOSHOTGUNBLOOD;
		aimoffset 32;
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("TURR_RECOG");
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.counter >= 48)
		{
			self.counter = 0;
		}
		else if (self.counter >= 32)
		{
				self.actorsizeto(32 * REPEAT_SCALE, 32 * REPEAT_SCALE);
		}
		else
		{
			if (self.counter >= 16)
			{
				self.actorsizeto(48 * REPEAT_SCALE, 18 * REPEAT_SCALE);
				if (pdist < 2048 * maptoworld)
				{
					self.PlayActorSound("TURR_ATTACK");
					p.addphealth(-2, self.bBIGHEALTH);
					p.pals = color(32, 16, 0, 0);
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.addkill();
				self.PlayActorSound("TURR_DYING");
				self.spawnguts('DukeJibs5', 10);
				self.killit();
			}
			self.PlayActorSound("TURR_PAIN");
			return;
		}
		if (Duke.rnd(1))
		{
			self.PlayActorSound("TURR_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
	}
}