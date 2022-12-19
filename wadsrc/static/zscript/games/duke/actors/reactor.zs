class DukeReactor : DukeActor
{
	default
	{
		spriteset "REACTOR", "REACTORBURNT";
	}
		

	override void Initialize()
	{
		self.extra = gs.impact_damage;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		self.shade = -17;
		self.pal = 0;
		if (ud.multimode < 2 && self.pal != 0)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else self.ChangeStat(STAT_ZOMBIEACTOR);
	}


	override void Tick()
	{
		if (self.spritesetindex == 1) return;	// this one's already dead.
		let sectp = self.sector;

		if (self.temp_data[4] == 1)
		{
			DukeSectIterator it;
			for (let a2 = it.First(self.sector); a2; a2 = it.Next())
			{
				if (a2.statnum == STAT_EFFECTOR)
				{
					if (a2.lotag == SE_1_PIVOT)
					{
						a2.lotag = -1;
						a2.hitag = -1;
					}
				}
				else if (a2 is "DukeReactor")
				{
					a2.SetSpritesetImage(1);
				}
				else if (a2 is "DukeReactorSpark")
				{
					a2.cstat = CSTAT_SPRITE_INVISIBLE;
				}
			}		
			return;
		}

		if (self.temp_data[1] >= 20)
		{
			self.temp_data[4] = 1;
			return;
		}

		double xx;
		DukePlayer p;
		[p, xx] = self.FindPlayer();

		// This was defined for REACTOR2, which doesn't have any animation frames
		//self.temp_data[2]++;
		//if (self.temp_data[2] == 4) self.temp_data[2] = 0;
		//self.SetSpriteSetImage(self.temp_data[2]);

		if (xx < 256)
		{
			if (random(0, 255) < 16)
			{
				if (!p.actor.CheckSoundPlaying("PLAYER_LONGTERM_PAIN"))
					p.actor.PlayActorSound("PLAYER_LONGTERM_PAIN");

				self.PlayActorSound("SHORT_CIRCUIT");

				p.actor.extra--;
				p.pals = Color(32, 32, 0, 0);
			}
			self.temp_data[0] += 128;
			if (self.temp_data[3] == 0)
				self.temp_data[3] = 1;
		}
		else self.temp_data[3] = 0;

		if (self.temp_data[1])
		{
			self.temp_data[1]++;

			double zsave = self.pos.Z;
			self.pos.Z = sectp.floorz - frandom(0, sectp.floorz - sectp.ceilingz);

			switch (self.temp_data[1])
			{
			case 3:
			{
				//Turn on all of those flashing sectoreffector.
				self.hitradius(4096,
					gs.impact_damage << 2,
					gs.impact_damage << 2,
					gs.impact_damage << 2,
					gs.impact_damage << 2);
				DukeStatIterator it;
				for (let a2 = it.First(STAT_STANDABLE); a2; a2 = it.Next())
				{
					if (a2 is 'DukeMasterSwitch' && a2.hitag == self.hitag && a2.yint == 0)
						a2.yint = 1;
				}
				break;
			}
			case 4:
			case 7:
			case 10:
			case 15:
			{
				DukeSectIterator it;
				for (let a2 = it.First(self.sector); a2; a2 = it.Next())
				{
					if (a2 != self)
					{
						a2.Destroy();
						break;
					}
				}
				break;
			}
			}
			for (int x = 0; x < 16; x++)
				self.RANDOMSCRAP();

			self.pos.Z = zsave;
			self.temp_data[4] = 0;

		}
		else
		{
			int j = self.ifhitbyweapon();
			if (j >= 0)
			{
				for (int x = 0; x < 32; x++)
					self.RANDOMSCRAP();
				if (self.extra < 0)
					self.temp_data[1] = 1;
			}
		}
	}
}

class DukeReactor2 : DukeReactor
{
	default
	{
		spriteset "REACTOR2", "REACTOR2BURNT";
	}
}

// These are inert and only needed for checking above.
class DukeReactorSpark : DukeActor
{
	default
	{
		pic "REACTORSPARK";
	}
}

class DukeReactor2Spark : DukeActor
{
	default
	{
		pic "REACTOR2SPARK";
	}
}
