class DukeWatersplash : DukeActor
{
	default
	{
		spriteset "WATERSPLASH2", "WATERSPLASH2A", "WATERSPLASH2B", "WATERSPLASH2C", "WATERSPLASH2D";
		+NOTELEPORT;
	}

	override void Initialize(DukeActor spawner)
	{
		let sectp = self.sector;
		if (spawner)
		{
			self.SetPosition(spawner.pos);
			double s = 0.125 + random(0, 7) * REPEAT_SCALE;
			self.scale = (s, s);
		}
		else
		{
			double s = 0.25 + random(0, 15) * REPEAT_SCALE;
			self.scale = (s, s);
		}

		self.shade = -16;
		self.cstat |= CSTAT_SPRITE_YCENTER;
		if (spawner)
		{
			double c, f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			if (spawner.sector.lotag == ST_2_UNDERWATER)
			{
				self.pos.Z = c + 16;
				self.cstat |= CSTAT_SPRITE_YFLIP;
			}
			else if (spawner.sector.lotag == ST_1_ABOVE_WATER)
				self.pos.Z = f;
		}

		if ((dlevel.floorsurface(sectp) == Duke.TSURF_SLIME) || (dlevel.ceilingsurface(sectp) & Duke.TSURF_SLIME))
			self.pal = 7;
		self.ChangeStat(STAT_MISC);
	}
	
	void DoTick(bool check)
	{
		let sectp = self.sector;
		self.counter++;
		if (self.counter == 1)
		{
			if (check)
			{
				self.Destroy();
				return;
			}
			if (!Duke.CheckSoundPlaying("ITEM_SPLASH"))
				self.PlayActorSound("ITEM_SPLASH");
		}
		if (self.counter == 3)
		{
			self.counter = 0;
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
		+NOTELEPORT;
	}

	override void Initialize(DukeActor spawner)
	{
		Super.Initialize(spawner);
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}

	override void Tick()
	{
		let sectp = self.sector;
		DoTick(dlevel.floorsurface(sectp) & Duke.TSURF_MUDDY);
	}
}

