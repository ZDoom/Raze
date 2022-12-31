// This appears not to be used
class RedneckPigDisplay : DukeActor
{
	default
	{
		spriteset "PIGBACK", "PIGBACK1", "PIGBACK2", "PIGBACK3", "PIGBACK4";
	}
	
	override void Initialize()
	{
		cstat = 0;
		self.scale = (0.25, 0.25);
		self.clipdist = 0;
		self.extra = 0;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		self.extra++;
		if (self.extra < 100)
		{
			if (self.extra == 90)
			{
				self.setSpriteSetImage(min(self.spritesetIndex + 1, 4));
				self.extra = 1;
			}
			self.movesprite((0, 0, -300/256.), CLIPMASK0);
			if (self.sector.ceilingz + 4 > self.pos.Z)
			{
				self.cstat = CSTAT_SPRITE_INVISIBLE;
				self.extra = 100;
			}
		}
		else if (self.extra == 200)
		{
			// This was really 10 and not (10 << 8)!
			self.SetPosition((self.pos.X, self.pos.Y, self.sector.floorz - 10 * zmaptoworld));
			self.extra = 1;
			self.cstat = 0;
			self.setSpriteSetImage(0);
			self.spawn("DukeTransporterStar");
		}
	}
}

