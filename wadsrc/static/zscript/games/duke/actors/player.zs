class DukePlayerPawn : DukeActor
{
	default
	{
		pic "APLAYER";
	}
}

class DukePlayerOnWater : DukeActor
{
	default
	{
		pic "PLAYERONWATER";
		+ALWAYSROTATE1;
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

	override void OnHit(DukeActor proj)
	{
		// propagate the hit to its owner.
		let owner = self.ownerActor;
		if (owner && self != owner) owner.OnHit(proj);
	}

}

class DukePlayerLyingDead : DukeActor
{
	default
	{
		pic "DUKELYINGDEAD";
		+HITRADIUS_FLAG2;
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
