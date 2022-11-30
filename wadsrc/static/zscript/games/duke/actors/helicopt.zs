class DukeCar : DukeActor
{
	default
	{
		extra 1;
		clipdist 32;
		pic "DUKECAR";
		statnum STAT_ACTOR;
	}
	
	override void Initialize()
	{
		self.vel.X = 292 / 16.;
		self.vel.Z = 360 / 256.;
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
	}
	
	override void Tick()
	{
		self.pos.Z += self.vel.Z;
		self.temp_data[0]++;

		if (self.temp_data[0] == 4) self.PlayActorSound("WAR_AMBIENCE2");

		if (self.temp_data[0] > (26 * 8))
		{
			Duke.PlaySound("RPG_EXPLODE");
			for (int j = 0; j < 32; j++) 
					self.RANDOMSCRAP();
			ud.earthquaketime = 16;
			self.Destroy();
			return;
		} 
		else if ((self.temp_data[0] & 3) == 0)
			self.spawn("DukeExplosion2");
		self.DoMove(CLIPMASK0);
	}
}

class DukeHelicopter : DukeCar
{
	default
	{
		pic "HELECOPT";
	}
}
