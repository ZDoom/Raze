

class DukeRat : DukeActor
{
	default
	{
		pic "RAT";
		+INTERNAL_BADGUY;
		+ALWAYSROTATE1;
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned) self.lotag = 0;

		self.makeitfall();
		self.angle = frandom(0, 360);
		self.cstat = 0;

		if (!self.mapSpawned && self.ownerActor)
		{
			self.timetosleep = 0;
			self.ChangeStat(STAT_ACTOR);
			if (isRR()) self.shade = self.ownerActor.shade;
		}
		else self.ChangeStat(STAT_ZOMBIEACTOR);
		self. clipdist = 10;
		self. scale = (0.75, 0.75);
	}

	override void Tick()
	{
		self.makeitfall();
		if (self.DoMove(CLIPMASK0))
		{
			if (!isRRRA() && random(0, 255) == 0) self.PlayActorSound("RATTY");
			self.angle += Raze.BAngToDegree * (random(-15, 15) + Raze.BobVal(self.counter << 8) * 8);
		}
		else
		{
			self.counter++;
			if (self.counter > 1)
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
