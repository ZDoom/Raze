class DukeBolt1 : DukeActor
{
	default
	{
		yint 0;
		statnum STAT_STANDABLE;
		SpriteSet "BOLT1", "BOLT2", "BOLT3", "BOLT4";
		spritesetindex 0;
	}
	override void Initialize()
	{
		self.temp_pos.XY = self.scale;
	}

	override void Tick()
	{
		DukePlayer p;
		double xx;
		let sectp = self.sector;

		[p, xx] = self.findplayer();
		if (xx > 1280) return;

		if (self.temp_data[3] == 0)
			self.temp_data[3] = sectp.floorshade;

		while (true)
		{
			if (self.temp_data[2])
			{
				self.temp_data[2]--;
				sectp.floorshade = 20;
				sectp.ceilingshade = 20;
				return;
			}
			if (self.scale == (0, 0))
			{
				self.scale = self.temp_pos.XY;
			}
			else if ((random() & 8) == 0)
			{
				self.temp_pos.XY = self.scale;
				self.temp_data[2] = Duke.global_random() & 4;
				self.scale = (0, 0);
				continue;
			}
			break;
		}
		self.SetSpritesetImage((self.spritesetindex + 1) % self.GetSpriteSetSize());

		int l = Duke.global_random() & 7;
		self.scale.X = (0.125 + l * REPEAT_SCALE);

		if (l & 1) self.cstat ^= CSTAT_SPRITE_TRANSLUCENT;

		if (self.spritesetindex == 1 && random(0, 7) == 0 && (dlevel.floorflags(sectp) & Duke.TFLAG_ELECTRIC))
			self.PlayActorSound("SHORT_CIRCUIT");

		if (self.spritesetindex & 1)
		{
			sectp.floorshade = 0;
			sectp.ceilingshade = 0;
		}
		else
		{
			sectp.floorshade = 20;
			sectp.ceilingshade = 20;
		}
		
	}
}

class DukeBolt2 : DukeBolt1
{
	default
	{
		spritesetindex 1;
	}
}

class DukeBolt3 : DukeBolt1
{
	default
	{
		spritesetindex 2;
	}
}

class DukeBolt4 : DukeBolt1
{
	default
	{
		spritesetindex 3;
	}
}

class DukeSideBolt1 : DukeBolt1
{
	default
	{
		SpriteSet "SIDEBOLT1", "SIDEBOLT2", "SIDEBOLT3", "SIDEBOLT4";
	}
	
	override void Tick()
	{
		DukePlayer p;
		double xx;
		let sectp = self.sector;

		[p, xx] = self.findplayer();
		if (xx > 1280) return;

		while (true)
		{
			if (self.temp_data[2])
			{
				self.temp_data[2]--;
				return;
			}
			if (self.scale == (0, 0))
			{
				self.scale = self.temp_pos.XY;
			}
			if ((random() & 8) == 0)
			{
				self.temp_pos.XY = self.scale;
				self.temp_data[2] = Duke.global_random() & 4;
				self.scale = (0, 0);
				continue;
			}
			break;
		}
		self.SetSpriteSetImage((self.spritesetindex + 1) % self.GetSpriteSetSize());

		if (random(0, 1) && (dlevel.floorflags(sectp) & Duke.TFLAG_ELECTRIC))
			self.PlayActorSound("SHORT_CIRCUIT");
	}
	
}

class DukeSideBolt2 : DukeSideBolt1
{
	default
	{
		spritesetindex 1;
	}
}

class DukeSideBolt3 : DukeSideBolt1
{
	default
	{
		spritesetindex 2;
	}
}

class DukeSideBolt4 : DukeSideBolt1
{
	default
	{
		spritesetindex 3;
	}
}

