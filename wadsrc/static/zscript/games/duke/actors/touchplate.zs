
class DukeTouchPlate : DukeActor
{
	default
	{
		statnum STAT_STANDABLE;
	}

	private bool checkspawn()
	{
		if (!Raze.isWorldTour())
		{
			if (self.pal && ud.multimode > 1) return false;
		}
		else { // Twentieth Anniversary World Tour addition - needs to be guarded because some mods surely will run afoul of it
			if ((self.pal == 1 && ud.multimode > 1) // Single-game Only
				|| (self.pal == 2 && (ud.multimode == 1 || (ud.multimode > 1 && ud.coop != 1))) // Co-op Only
				|| (self.pal == 3 && (ud.multimode == 1 || (ud.multimode > 1 && ud.coop == 1)))) // Dukematch Only
			{
				return false;
			}
		}
		return true;
	}

	override void Initialize()
	{
		let sectp = self.sector;
		self.temp_data[2] = sectp.floorz;
		if (sectp.lotag != 1 && sectp.lotag != 2)
			sectp.setfloorz(self.pos.Z);
		if (!checkspawn())
		{
			self.Scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
	}


	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void Tick()
	{
		let sectp = self.sector;
		DukePlayer p;

		if (self.temp_data[1] == 1 && self.hitag >= 0) //Move the sector floor
		{
			double Z = sectp.floorz;
			double add = sectp.extra * zmaptoworld;

			if (self.temp_data[3] == 1)
			{
				if (Z >= self.temp_pos.Z)
				{
					sectp.setfloorz(Z);
					self.temp_data[1] = 0;
				}
				else
				{
					sectp.addfloorz(add);
					p = Duke.checkcursectnums(sectp);
					if (p != null) p.addpos((0, 0, add));
				}
			}
			else
			{
				if (Z <= self.pos.Z)
				{
					sectp.setfloorz(self.pos.Z);
					self.temp_data[1] = 0;
				}
				else
				{
					sectp.addfloorz(-add);
					p = Duke.checkcursectnums(sectp);
					if (p != null) p.addpos((0, 0, -add));
				}
			}
			return;
		}

		if (self.temp_data[5] == 1) return;

		p = Duke.checkcursectnums(sectp);
		if (p != null && (p.on_ground || self.intangle == 512))
		{
			if (self.temp_data[0] == 0 && !dlevel.check_activator_motion(self.lotag))
			{
				self.temp_data[0] = 1;
				self.temp_data[1] = 1;
				self.temp_data[3] = !self.temp_data[3];
				dlevel.operatemasterswitches(self.lotag);
				dlevel.operateactivators(self.lotag, p);
				if (self.hitag > 0)
				{
					self.hitag--;
					if (self.hitag == 0) self.temp_data[5] = 1;
				}
			}
		}
		else self.temp_data[0] = 0;

		if (self.temp_data[1] == 1)
		{
			DukeStatIterator it;
			for(let act2 = it.first(STAT_STANDABLE); act2; act2 = it.Next())
			{
				if (act2 != self && act2 is 'DukeTouchPlate' && act2.lotag == self.lotag)
				{
					act2.temp_data[1] = 1;
					act2.temp_data[3] = self.temp_data[3];
				}
			}
		}
	}
}
