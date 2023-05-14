class DukePlayerPawn : DukePlayerBase
{
	default
	{
		Strength MAXPLAYERHEALTH;
	}
	
}


class DukePlayerOnWater : DukeActor
{
	default
	{
		pic "PLAYERONWATER";
		+ALWAYSROTATE1;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner)
		{
				self.scale = spawner.scale;
				self.vel.Z = 0.5;
				if (self.sector.lotag != ST_2_UNDERWATER)
					self.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		self.ChangeStat(STAT_DUMMYPLAYER);
	}

	override void OnHit(DukeActor proj)
	{
		// propagate the hit to its Owner.
		let Owner = self.OwnerActor;
		if (Owner && self != Owner) Owner.OnHit(proj);
	}

}

class DukePlayerLyingDead : DukeActor
{
	default
	{
		pic "DUKELYINGDEAD";
		+HITRADIUS_FORCEEFFECT;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner && spawner.isPlayer())
		{
			self.scale = spawner.scale;
			self.shade = spawner.shade;
			self.pal = spawner.GetPlayer().palookup;
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
