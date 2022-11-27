

class DukeRat : DukeActor
{
	default
	{
		pic "RAT";
		clipdist 10;
		scaleX 0.75;
		scaleY 0.75;
	}
	
	override void Initialize()
	{
		if (ownerActor) self.lotag = 0;

		if ((self.lotag > ud.player_skill) || ud.monsters_off == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else
		{
			self.makeitfall();
			self.angle = frandom(0, 360);
			self.cstat = 0;

			if (self.ownerActor)
			{
				self.timetosleep = 0;
				self.ChangeStat(STAT_ACTOR);
				if (Raze.isRR()) self.shade = self.ownerActor.shade;
			}
			else self.ChangeStat(STAT_ZOMBIEACTOR);

		}
	}

	override void Tick()
	{
		self.makeitfall();
		if (self.DoMove(CLIPMASK0))
		{
			if (!Raze.isRRRA() && random(0, 255) == 0) self.PlayActorSound("RATTY");
			self.angle += Raze.BAngToDegree * (random(-15, 15) + Raze.BobVal(self.temp_data[0] << 8) * 8);
		}
		else
		{
			self.temp_data[0]++;
			if (self.temp_data[0] > 1)
			{
				self.Destroy();
				return;
			}
			else self.angle = frandom(0, 360);
		}
		if (self.vel.X < 8)
			self.vel.X += 1/8.;
		self.angle += Raze.BAngToDegree * (random(0, 3) - 6);
	}
}
