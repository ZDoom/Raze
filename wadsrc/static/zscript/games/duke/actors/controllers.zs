
class DukeActivator : DukeActor
{
	default
	{
		statnum STAT_ACTIVATOR;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
	}
	
	/* this first needs work on the sector effectors.
	override void onActivate(int low, DukePlayer plr)
	{
		switch (self.hitag)
		{
		case 0:
			break;
		case 1:
			if (self.sector.floorz != self.sector.ceilingz)
			{
				continue;
			}
			break;
		case 2:
			if (self.sector.floorz == self.sector.ceilingz)
			{
				continue;
			}
			break;
		}

		if (self.sector.lotag < 3)
		{
			DukeSectIterator itr;
			for(let a2 = itr.First(self.sector); a2; a2 = itr.Next())
			{
				// todo: move this into the effectors as a virtual override.
				if (a2.statnum == STAT_EFFECTOR) switch (a2.lotag)
				{
				case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
					if (Raze.isRRRA()) break;

				case SE_36_PROJ_SHOOTER:
				case SE_31_FLOOR_RISE_FALL:
				case SE_32_CEILING_RISE_FALL:
					a2.temp_data[0] = 1 - a2.temp_data[0];
					a2.callsound(self.sector());
					break;
				}
			}
		}

		if (k == -1 && (self.sector.lotag & 0xff) == SE_22_TEETH_DOOR)
			k = act.callsound(self.sector);

		self.operatesectors(self.sector);
	}
	*/
}

class DukeLocator : DukeActor
{
	default
	{
		statnum STAT_LOCATOR;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
	}
}

class DukeActivatorLocked : DukeActor
{
	default
	{
		statnum STAT_ACTIVATOR;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		if (!Raze.IsRR()) self.lotag |= 16384;
		else self.lotag ^= 16384;
	}
	
	/* must wait until Activator.onActivate can be done.
	override void onActivate(int low, DukePlayer plr)
	{
		if (self.lotag == low)
		{
			self.sector.lotag ^= 16384;

			if (plr)
			{
				if (self.sector.lotag & 16384)
					plr.FTA(QUOTE_LOCKED, true);
				else plr.FTA(QUOTE_UNLOCKED, true);
			}
		}
	}
	*/
}

// Note: StaticSetup is run much earlier than Initialize! This is only meant for things that modify global game state.

class DukeCycler : DukeActor
{
	override void StaticSetup()
	{
		dlevel.AddCycler(self.sector, self.lotag, self.shade, self.sector.floorshade, self.hitag, self.intangle == 1536);
		self.Destroy();
	}
}

class DukeGPSpeed : DukeActor
{
	override void StaticSetup()
	{
		self.sector.extra = self.lotag;
		self.Destroy();
	}
}

// the following ones are only used in RR.
class TorchCtrl : DukeActor
{
	override void StaticSetup()
	{
		dlevel.addtorch(self.sector, self.sector.floorshade, self.lotag);
		self.Destroy();
	}
}

class DukeLightningCtrl : DukeActor
{
	override void StaticSetup()
	{
		dlevel.addlightning(self.sector, self.sector.floorshade);
		self.Destroy();
	}
}

class DukeShadeCtrl : DukeActor
{
	override void StaticSetup()
	{
		self.sector.shadedsector = 1;
		self.Destroy();
	}
}

class DukeDummyCtrl : DukeActor
{
	override void StaticSetup()
	{
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
	}
}

class DukeSoundFX : DukeActor
{
	default
	{
		statnum STAT_ZOMBIEACTOR;
	}
	
	override void StaticSetup()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.detail = dlevel.addambient(self.hitag, self.lotag);
		self.lotag = self.hitag = 0;
	}
}

