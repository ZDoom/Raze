
class DukeScrap : DukeActor
{
	default
	{
		statnum STAT_MISC;
	}
	
	enum EScrap
	{
		Scrap6 = 0,
		Scrap1 = 10,
		Scrap2 = 14,
		Scrap3 = 18,
		Scrap4 = 22,
		Scrap5 = 26,
		ScrapMax = 30
	}
	
	default
	{
		spriteset
		"SCRAP6", "SCRAP6A", "SCRAP6B", "SCRAP6C", "SCRAP6D", "SCRAP6E", "SCRAP6F", "SCRAP6G", "SCRAP6H", "SCRAP6I",
		"SCRAP1", "SCRAP1A", "SCRAP1B", "SCRAP1C",
		"SCRAP2", "SCRAP2A", "SCRAP2B", "SCRAP2C",
		"SCRAP3", "SCRAP3A", "SCRAP3B", "SCRAP3C",
		"SCRAP4", "SCRAP4A", "SCRAP4B", "SCRAP4C",
		"SCRAP5", "SCRAP5A", "SCRAP5B", "SCRAP5C";
	}
	
	static const int8 brighter[] = { 1,1,1,1,1,1,1,1,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0};	// this is one of those cases where Duke's switch/case mess left handling incomplete.

	override void Tick()
	{
		let sectp = self.sector;

		if(self.vel.X > 0)
			self.vel.X -= 1/16.;
		else self.vel.X = 0;

		if (self.vel.Z > 4 && self.vel.Z < 5)
		{
			self.SetPosition(self.pos);
			sectp = self.sector;
		}

		if (self.pos.Z < sectp.floorz - 2)
		{
			if (self.temp_data[1] < 1) self.temp_data[1]++;
			else
			{
				self.temp_data[1] = 0;

				if (self.spriteextra < Scrap6 + 8)
				{
					if (self.temp_data[0] > 6)
						self.temp_data[0] = 0;
					else self.temp_data[0]++;
				}
				else
				{
					if (self.temp_data[0] > 2)
						self.temp_data[0] = 0;
					else self.temp_data[0]++;
				}
			}
			if (self.vel.Z < 16) self.vel.Z += (gs.gravity - 50 / 256.);
			self.pos += self.angle.ToVector() * self.vel.X;
			self.pos.Z += self.vel.Z;
		}
		else
		{
			if (self.spriteextra == Scrap1 && self.yint > 0)
			{
				let spawned = self.spawnweaponorammo(self.yint - 1); // fixme later!
				if (spawned)
				{
					spawned.SetPosition(self.pos);
					spawned.getglobalz();
					spawned.hitag = spawned.lotag = 0;
				}
			}
			self.Destroy();
		}
	}
	
	override bool animate(tspritetype tspr)
	{
		if (self.spriteextra == Scrap1 && self.yint > 0)
		{
			tspr.setWeaponOrAmmoSprite(self.yint - 1); // needed so that we don't have to export tile numbers to scripting.
		}
		else 
		{
			let frame = self.spriteextra + self.temp_data[0];
			Console.Printf("extra = %d, temp = %d", self.spriteextra, self.temp_data[0]);
			if (frame < 0 || frame >= ScrapMax) frame = Scrap3;
			tspr.setSpritePic(self, frame);
			if (brighter[frame]) tspr.shade -= 6;
		}
		return true;
	}
	
}

