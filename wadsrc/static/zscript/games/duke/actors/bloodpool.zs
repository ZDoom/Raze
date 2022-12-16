class DukeBloodPool : DukeActor
{
	default
	{
		pic "BLOODPOOL";
		+BLOODY;
		+SE24_REMOVE;
	}
	
	virtual void SetPalette()
	{
		let Owner = self.ownerActor;
		if (Owner && !mapSpawned)
		{
			if (Owner.pal == 1)
				self.pal = 1;	// Blue
			else if (Owner.pal == 6 || Owner.actorflag2(SFLAG2_GREENBLOOD))
			{
				self.pal = 0;	// Green
				self.temp_data[5] = 1;	// this hurts!
			}
			else if (Owner.actorflag3(SFLAG3_BROWNBLOOD))
				self.pal = 7; 	// Brown
			else
				self.pal = 2; // Red
		}
	}

	override void Initialize()
	{
		if (!checkLocationForFloorSprite(6.75)) return;
		SetPalette();
		self.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		if (!self.mapSpawned)
			self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.ChangeStat(STAT_MISC);
	}
	
	override void Tick()
	{
		let sectp = self.sector;

		if (self.temp_data[0] == 0)
		{
			self.temp_data[0] = 1;
			if (sectp.floorstat & CSTAT_SECTOR_SLOPE)
			{
				self.Destroy();
				return;
			}
			else self.insertspriteq();
		}
		if (Raze.isRR() && self.sector.lotag == 800)
		{
			self.Destroy();
			return;
		}

		self.makeitfall();

		double xx;
		DukePlayer plr;
		[plr, xx] = self.findplayer();

		self.pos.Z = self.floorz - 0.125;

		if (self.temp_data[2] < 32)
		{
			self.temp_data[2]++;
			if (self.detail == 1)
			{
				if (self.scale.X < 1 && self.scale.Y < 1)
				{
					self.scale.X += (random(0, 3) * REPEAT_SCALE);
					self.scale.Y += (random(0, 3) * REPEAT_SCALE);
				}
			}
			else
			{
				if (self.scale.X < 0.5 && self.scale.Y < 0.5)
				{
					self.scale.X += (random(0, 3) * REPEAT_SCALE);
					self.scale.Y += (random(0, 3) * REPEAT_SCALE);
				}
			}
		}

		if (xx < 844 / 16. && self.scale.X > 0.09375 && self.scale.Y > 0.09375)
		{
			if (random(0, 256) < 16 && self.temp_data[5])
			{
				if (plr.boot_amount > 0)
					plr.boot_amount--;
				else
				{
					if (!plr.actor.CheckSoundPlaying("PLAYER_LONGTERM_PAIN"))
						plr.actor.PlayActorSound("PLAYER_LONGTERM_PAIN");
					plr.actor.extra--;
					plr.pals = Color(32, 16, 0, 0);
				}
			}

			if (self.temp_data[1] == 1) return;
			self.temp_data[1] = 1;

			if (self.detail == 1)
				plr.footprintcount = 10;
			else plr.footprintcount = 3;

			plr.footprintpal = self.pal;
			plr.footprintshade = self.shade;

			if (self.temp_data[2] == 32)
			{
				self.scale.X += (-0.09375);
				self.scale.Y += (-0.09375);
			}
		}
		else self.temp_data[1] = 0;
	}

	override bool Animate(tspritetype t)
	{
		if (self.shade == 127) t.shade = self.shade;
		if (t.pal == 6) t.shade = -127;
		return true;
	}
}


class DukePuke : DukeBloodPool
{
	default
	{
		pic "PUKE";
	}
	
	override bool Animate(tspritetype t)
	{
		if (self.shade == 127) t.shade = self.shade;
		return true;
	}
}
