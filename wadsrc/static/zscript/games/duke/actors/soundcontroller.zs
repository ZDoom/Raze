

class DukeSoundController : DukeActor
{
	override void Initialize()
	{
		if (ud.multimode < 2 && self.pal == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.ChangeStat(STAT_FX);
	}
	
	override void Tick()
	{
		double maxdist = self.hitag * maptoworld;

		if (self.temp_data[1] != Raze.SoundEnabled())
		{
			self.temp_data[1] = Raze.SoundEnabled();
			self.counter = 0;
		}

		let p = Duke.GetViewPlayer();
		if (self.lotag >= 1000 && self.lotag < 2000)
		{
			double dist = (p.actor.pos.XY - self.pos.XY).LengthSquared();
			if (dist < maxdist * maxdist && self.counter == 0)
			{
				Raze.SetReverb(self.lotag - 1100);
				self.counter = 1;
			}
			if (dist >= maxdist * maxdist && self.counter == 1)
			{
				Raze.SetReverb(0);
				Raze.SetReverbDelay(0);
				self.counter = 0;
			}
		}
		else 
		{
			let sec = self.sector;
			if (self.lotag < 999 && sec.lotag >= 0 && sec.lotag < ST_9_SLIDING_ST_DOOR && snd_ambience && sec.floorz != sec.ceilingz)
			{
				int flags = Duke.GetSoundFlags(Raze.FindSoundByResID(self.lotag));
				if (flags & Duke.SF_MSFX)
				{
					double distance = (p.actor.pos - self.pos).Length();

					if (distance < maxdist && self.counter == 0)
					{
						// Start playing an ambience sound.
						self.PlayActorSound(Raze.FindSoundByResID(self.lotag), CHAN_AUTO, CHANF_LOOP);
						self.counter = 1;  // AMBIENT_SFX_PLAYING
					}
					else if (distance >= maxdist && self.counter == 1)
					{
						// Stop playing ambience sound because we're out of its range.
						self.StopSound(self.lotag);
					}
				}

				if ((flags & (Duke.SF_GLOBAL | Duke.SF_DTAG)) == Duke.SF_GLOBAL)
				{
					if (self.temp_data[4] > 0) self.temp_data[4]--;
					else if (sec == p.actor.sector)
					{
						Duke.PlaySound(Raze.FindSoundByResID(self.lotag + uint(Duke.global_random()) % uint(self.hitag + 1)));
						self.temp_data[4] = 26 * 40 + (Duke.global_random() % (26 * 40));
					}
				}
			}
		}
	}
	
	override void OnDestroy()
	{
		if (self.counter == 1)
			self.StopSound(-1);
		Super.OnDestroy();
	}
}
