class DukePlayerOnWater : DukeActor
{
	default
	{
		pic "PLAYERONWATER";
	}
	
	override void Initialize()
	{
		if (!mapSpawned && self.ownerActor)
		{
				self.scale = self.ownerActor.scale;
				self.vel.Z = 0.5;
				if (self.sector.lotag != ST_2_UNDERWATER)
					self.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		self.ChangeStat(STAT_DUMMYPLAYER);
	}
}

class DukePlayerLyingDead : DukeActor
{
	default
	{
		pic "DUKELYINGDEAD";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner.isPlayer())
		{
			self.scale = owner.scale;
			self.shade = owner.shade;
			self.pal = owner.GetPlayer().palookup;
		}
		self.vel.X = 292 / 16.;
		self.vel.Z = 360 / 256.;
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.extra = 1;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override bool animate(tspritetype t)
	{
		t.pos.Z += 24;
		return false;
	}
}

class RedneckPlayerLyingDead : DukeActor
{
	override bool animate(tspritetype t)
	{
		t.scale = (0.375, 0.265625);
		if (self.extra > 0)
			t.pos.Z += 6;
		return false;
	}
}
