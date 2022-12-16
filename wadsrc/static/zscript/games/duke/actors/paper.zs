
class DukeMoney : DukeActor
{
	default
	{
		spriteset "MONEY", "MONEY1";
	}
	
	override void Initialize()
	{
		self.scale = (0.125, 0.125);
		self.temp_data[0] = random(0, 2047);
		self.cstat = self.randomFlip();
		self.angle = frandom(0, 360);
		self.ChangeStat(STAT_MISC);
	}

	override void Tick()
	{
		let sectp = self.sector;
		
		if (self.spriteSetIndex == 1)
		{
			double x;
			[x, self.floorz] = sectp.getSlopes(self.pos.XY);
			self.vel.Z = self.floorz;
			return;
		}

		self.vel.X = frandom(0, 0.5) + Raze.BobVal(self.temp_data[0]) * 2;
		self.temp_data[0] += random(0, 63);
		if ((self.temp_data[0] & 2047) > 512 && (self.temp_data[0] & 2047) < 1596)
		{
			if (sectp.lotag == ST_2_UNDERWATER)
			{
				if (self.vel.Z < 0.25)
					self.vel.Z += gs.gravity / 32. + frandom(0, 1/32.);
			}
			else
				if (self.vel.Z < 0.5625)
					self.vel.Z += gs.gravity / 32. + frandom(0, 1 / 32.);
		}

		self.DoMove(CLIPMASK0);

		if ( random(0, 3) == 0)
			self.SetPosition(self.pos);

		if (self.sector == null)
		{
			self.Destroy();
			return;
		}
		double x, l;
		[x, l] = sectp.getSlopes(self.pos.XY);


		if (self.pos.Z > l)
		{
			self.pos.Z = l;
			self.insertspriteq();
			self.setSpriteSetImage(1);

			DukeStatIterator it;
			for (let aa = it.First(STAT_MISC); aa; aa = it.Next())
			{
				if (aa.bBLOODY)
				{
					double dist = (aa.pos.XY - self.pos.XY).Length();
					if (dist < 348/16.)
					{
						self.pal = 2;
						break;
					}
				}
			}
		}
	}
}

class DukeMail : DukeMoney
{
	default
	{
		spriteset "MAIL", "MAIL1";
	}
}

class DukePaper : DukeMoney
{
	default
	{
		spriteset "PAPER", "PAPER1";
	}
}

class RedneckFeather : DukeMoney
{
	default
	{
		spriteset "FEATHER", "FEATHER1";
	}
	
	override void Tick()
	{
		Super.Tick();
		if (self.sector.lotag == 800)
		{
			if ((self.pos.Z >= self.sector.floorz - 8) || self.spritesetindex == 1)
				self.Destroy();
		}
	}
		
}

