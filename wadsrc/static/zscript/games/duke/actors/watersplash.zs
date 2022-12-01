class DukeWatersplash : DukeActor
{
	default
	{
		shade -16;
		statnum STAT_MISC;
		spriteset "WATERSPLASH2", "WATERSPLASH2A", "WATERSPLASH2B", "WATERSPLASH2C", "WATERSPLASH2D";
	}

	override void Initialize()
	{
		let Owner = self.ownerActor;
		let sectp = self.sector;
		if (Owner)
		{
			self.SetPosition(Owner.pos);
			double s = 0.125 + random(0, 7) * REPEAT_SCALE;
			self.scale = (s, s);
		}
		else
		{
			double s = 0.25 + random(0, 15) * REPEAT_SCALE;
			self.scale = (s, s);
		}

		self.cstat |= CSTAT_SPRITE_YCENTER;
		if (Owner)
		{
			double c, f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			if (Owner.sector.lotag == ST_2_UNDERWATER)
			{
				self.pos.Z = c + 16;
				self.cstat |= CSTAT_SPRITE_YFLIP;
			}
			else if (Owner.sector.lotag == ST_1_ABOVE_WATER)
				self.pos.Z = f;
		}

		if ((dlevel.floorflags(sectp) & Duke.TFLAG_SLIME) || (dlevel.ceilingflags(sectp) & Duke.TFLAG_SLIME))
			self.pal = 7;
		self.ChangeStat(STAT_MISC);
	}
	
	void DoTick(bool check)
	{
		let sectp = self.sector;
		self.temp_data[0]++;
		if (self.temp_data[0] == 1)
		{
			if (check)
			{
				self.Destroy();
				return;
			}
			if (!Duke.CheckSoundPlaying("ITEM_SPLASH"))
				self.PlayActorSound("ITEM_SPLASH");
		}
		if (self.temp_data[0] == 3)
		{
			self.temp_data[0] = 0;
			self.temp_data[1]++;
		}
		if (self.temp_data[1] == 5)
		{
			self.Destroy();
			return;
		}
		
		self.setspritesetImage(self.temp_data[1]);
	}
		
	override void Tick()
	{
		let sectp = self.sector;
		DoTick(sectp.lotag != ST_1_ABOVE_WATER && sectp.lotag != ST_2_UNDERWATER);
	}
}

class RedneckMudSplash : DukeWatersplash
{
	default
	{
		spriteset "MUD", "MUD1", "MUD2", "MUD3", "MUD4";
	}

	override void Initialize()
	{
		Super.Initialize();
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}

	override void Tick()
	{
		let sectp = self.sector;
		DoTick(dlevel.floorflags(sectp) & Duke.TFLAG_MUDDY);
	}
}

