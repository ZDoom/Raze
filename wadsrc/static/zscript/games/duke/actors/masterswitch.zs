class DukeMasterSwitch : DukeActor
{
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.yint = 0;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void Tick()
	{
		if (self.yint == 1 && self.statnum == STAT_STANDABLE)
		{
			self.hitag--;
			if (self.hitag <= 0)
			{
				self.operatesectors(self.sector);

				DukeSectIterator it;
				for (let effector = it.First(self.sector); effector; effector = it.Next())
				{
					if (effector.statnum == STAT_EFFECTOR)
					{
						// Todo: Once the effectors have been ported, do this with a virtual that can be overridden by custom effects.
						switch (effector.lotag)
						{
						case SE_2_EARTHQUAKE:
						case SE_21_DROP_FLOOR:
						case SE_31_FLOOR_RISE_FALL:
						case SE_32_CEILING_RISE_FALL:
						case SE_36_PROJ_SHOOTER:
							effector.counter = 1;
							break;
						case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
							effector.temp_data[4] = 1;
							break;
						}
					}
					else //if (effector.statnum == STAT_STANDABLE) this check is not really needed.
					{
						if (effector.bBRIGHTEXPLODE) // SEENINE and OOZFILTER
						{
							effector.shade = -31;
						}
					}
				}
				// we cannot delete this actor because it may be used as a sound source.
				// This originally depended on undefined behavior as the deleted sprite was still used for the sound
				// with no checking if it got reused in the mean time.
				self.cstat |= CSTAT_SPRITE_INVISIBLE;
				self.cstat2 |= CSTAT2_SPRITE_NOFIND;
				self.ChangeStat(STAT_REMOVED);
			}
		}
	}
}
