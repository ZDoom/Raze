
class DukeShell : DukeActor
{
	default
	{
		spriteset "SHELL", "SHELL1";
	}

	void initshell(bool isshell, double direction)
	{
		let Owner = self.OwnerActor;
		if (Owner && !self.mapSpawned)
		{
			double ang;

			if (Owner.isPlayer())
			{
				let plr = Owner.GetPlayer();
				let pactor = plr.actor;
				ang = pactor.angle - Raze.BAngToDegree * (random(8, 71));  //Fine tune

				self.counter = random(0, 1);
				self.pos.Z = 3 + pactor.pos.Z + pactor.viewzoffset + plr.pyoff + tan(plr.getPitchWithView()) * 8. + (!isshell ? 3 : 0);
				self.vel.Z = -frandom(0, 1);
			}
			else
			{
				ang = self.angle;
				self.pos.Z = Owner.pos.Z - gs.playerheight + 3;
			}

			self.pos.XY = Owner.pos.XY + ang.ToVector() * 8;
			self.opos = self.pos;

			if (direction > 0)
			{
				// to the right, with feeling
				self.angle = ang + 90;
				self.vel.X = direction;
			}
			else
			{
				self.angle = ang - 90;
				self.vel.X = -direction;
			}

			double scale = isRR() && isshell ? 0.03125 : 0.0625;
			self.scale = (scale, scale);
		}
		self.shade = -8;
		self.ChangeStat(STAT_MISC);
	}
	
	override void Initialize()
	{
		initshell(true, -1.25);
	}

	override void Tick()
	{
		let sectp = self.sector;

		self.DoMove(CLIPMASK0);

		if (sectp == null || sectp.floorz + 24 < self.pos.Z)
		{
			self.Destroy();
			return;
		}

		if (sectp.lotag == ST_2_UNDERWATER)
		{
			self.temp_data[1]++;
			if (self.temp_data[1] > 8)
			{
				self.temp_data[1] = 0;
				self.counter++;
				self.counter &= 3;
			}
			if (self.vel.Z < 0.5) self. vel.Z += (gs.gravity / 13); // 8
			else self.vel.Z -= 0.25;
			if (self.vel.X > 0)
				self.vel.X -= 0.25;
			else self.vel.X = 0;
		}
		else
		{
			self.temp_data[1]++;
			if (self.temp_data[1] > 3)
			{
				self.temp_data[1] = 0;
				self.counter++;
				self.counter &= 3;
			}
			if (self.vel.Z < 2) self.vel.Z += (gs.gravity / 3); // 52;
			if(self.vel.X > 0)
				self.vel.X -= 1/16.;
			else
			{
				self.Destroy();
			}
		}
	}

	override bool animate(tspritetype t)
	{
		if (self.GetSpriteSetsize() > 0) t.setspritepic(self,  self.counter & 1);
		t.cstat |= (CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
		if (self.counter > 1) t.cstat &= ~CSTAT_SPRITE_XFLIP;
		if (self.counter > 2) t.cstat &= ~(CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
		return true;
	}
}

class NamShell : DukeShell
{
	override void Initialize()
	{
		initshell(true, 1.875);
	}
}

class DukeShotgunShell : DukeShell
{
	default
	{
		spriteset "SHOTGUNSHELL";
	}
}

class NamShotgunShell : NamShell
{
	default
	{
		spriteset "SHOTGUNSHELL";
	}
}
