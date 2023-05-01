
class DukeBurning : DukeActor
{
	default
	{
		pic "BURNING";
		+FULLBRIGHT;
		+FORCERUNCON;
		+NOTELEPORT;
		+NOFLOORPAL;
		action "BURNING_FLAME", 0, 12, 1, 1, 2;
		move "BURNING_VELS";
		Strength WEAK;
		StartAction "BURNING_FLAME";
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned)
		{
			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.Scale = (0.0625, 0.0625);
		self.ChangeStat(STAT_MISC);
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.timetosleep += 300;
		if (self.attackertype is 'DukeBurning')
		{
			if (self.floorz - self.ceilingz < 16)
			{
				return;
			}
		}
		if (pdist > 10240 * maptoworld)
		{
			return;
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.counter >= 128)
		{
			if (self.attackertype is 'RedneckFire') // how?
			{
				if (self.actioncounter >= 512)
				{
					self.killit();
				}
				if (Duke.rnd(16))
				{
					self.actorsizeto(64 * REPEAT_SCALE, 48 * REPEAT_SCALE);
				}
			}
			else
			{
				self.actorsizeto(8 * REPEAT_SCALE, 8 * REPEAT_SCALE);
				self.actorsizeto(8 * REPEAT_SCALE, 8 * REPEAT_SCALE);
				if (self.counter >= 192)
				{
					self.killit();
				}
			}
		}
		else
		{
			if (self.curMove.name == 'none')
			{
				setMove('BURNING_VELS', 0);
			}
			self.actorsizeto(52 * REPEAT_SCALE, 52 * REPEAT_SCALE);
			if (self.checkp(p, palive))
			{
				if (pdist < 844 * maptoworld)
				{
					if (Duke.rnd(32))
					{
						if (self.cansee(p))
						{
							self.PlayActorSound("PLAYER_LONGTERM_PAIN", CHAN_AUTO, CHANF_SINGULAR);
							p.addphealth(-1, self.bBIGHEALTH);
							p.pals = color(24, 16, 0, 0);
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}

	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		
		if (OwnerAc && OwnerAc.statnum == STAT_PLAYER)
		{
			let p = Duke.GetViewPlayer();
			if (display_mirror == 0 && OwnerAc == p.actor && p.over_shoulder_on == 0)
				t.scale = (0, 0);
			else
			{
				// not needed.
				//t.Angle = (viewVec - t.pos.XY()).Angle();
				//t.pos.XY = OwnerAc.pos.XY + t.Angle.ToVector();
			}
		}
		t.cstat |= CSTAT_SPRITE_YCENTER;
		double d;
		if (!bNOFLOORFIRE)
			[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
		return false;
	}
}

class DukeBurning2 : DukeBurning
{
	default
	{
		pic "BURNING2";
	}
}

class RedneckFire : DukeActor
{
	default
	{
		pic "FIRE";
		+FULLBRIGHT;
		+NOTELEPORT;
		+NOFLOORPAL;
		
		action "FIRE_FRAMES", -1, 14, 1, 1, 1;
		move "FIREVELS";
		Strength WEAK;
		StartMove "FIREVELS";
		
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'none')
		{
			if (Duke.rnd(64))	// this was 16 which can delay the flame for several seconds.
			{
				setAction('FIRE_FRAMES');
				self.cstat |= CSTAT_SPRITE_YCENTER;
			}
		}
		self.timetosleep += 300;
		if (self.attackertype is 'RedneckFire')
		{
			if (self.floorz - self.ceilingz < 16)
			{
				return;
			}
		}
		if (self.sector.lotag == ST_2_UNDERWATER)
		{
			self.killit();
		}
		if (self.checkp(p, palive))
		{
			if (pdist < 844 * maptoworld)
			{
				if (Duke.rnd(32))
				{
					if (self.cansee(p))
					{
						self.PlayActorSound("PLAYER_LONGTERM_PAIN", CHAN_AUTO, CHANF_SINGULAR);
						p.addphealth(-1, self.bBIGHEALTH);
						p.pals = color(32, 32, 0, 0);
					}
				}
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		}
		if (self.attackertype is 'RedneckFire')
		{
			return;
		}
		if (self.floorz - self.pos.Z < 128)
		{
			if (Duke.rnd(128))
			{
				if (self.counter >= 84)
				{
					self.killit();
				}
				else if (self.counter >= 42)
				{
					self.actorsizeto(0 * REPEAT_SCALE, 0 * REPEAT_SCALE);
				}
				else
				{
					self.actorsizeto(32 * REPEAT_SCALE, 32 * REPEAT_SCALE);
				}
			}
		}
		else
		{
			self.killit();
		}
	}
	
	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		double d;
		[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
		return false;
	}
}

class DukeFire : RedneckFire
{
	override bool animate(tspritetype t)
	{
		super.animate(t);
		t.cstat |= CSTAT_SPRITE_YCENTER;
		return false;
	}
}

class DukeFire2 : DukeFire
{
	default
	{
		pic "FIRE2";
	}
}

