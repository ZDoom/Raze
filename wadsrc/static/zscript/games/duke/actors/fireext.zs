
class DukeFireext : DukeActor
{
	default
	{
		statnum STAT_STANDABLE;
		pic "FIREEXT";
	}

	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.extra = gs.impact_damage << 2;

		if (ud.multimode < 2 && self.pal != 0)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}

		self.pal = 0;
		self.ownerActor = self;
		self.vel.X = 0.5;
		self.DoMove(CLIPMASK0);
	}
	
	override void Tick()
	{
		int j = self.ifhitbyweapon();
		if (j == -1) return;

		for (int k = 0; k < 16; k++)
		{
			let a = frandom(0, 360);
			let vel = frandom(0, 4) + 4;
			let zvel = -frandom(0, 16) - self.vel.Z * 0.25;
			let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(frandom(-48, 0)), 'DukeScrap', -8, (0.75, 0.75), a, vel, zvel, self);
			if (spawned)
			{
				if (spawned) spawned.spriteextra = DukeScrap.Scrap3 + random(0, 3);
				spawned.pal = 2;
			}
		}

		self.spawn("DukeExplosion2");
		self.PlayActorSound(DukeSnd.PIPEBOMB_EXPLODE);
		self.PlayActorSound(DukeSnd.GLASS_HEAVYBREAK);

		if (self.hitag > 0)
		{
			DukeStatIterator it;
			for(let a1 = it.First(STAT_STANDABLE); a1; a1 = it.Next())
			{
				if (self.hitag == a1.hitag && a1.actorflag2(SFLAG2_BRIGHTEXPLODE))
					if (a1.shade != -32)
						a1.shade = -32;
			}

			int x = self.extra;
			self.spawn("DukeExplosion2");
			self.hitradius(gs.pipebombblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
			self.PlayActorSound(DukeSnd.PIPEBOMB_EXPLODE);
			self.detonate("DukeExplosion2");
		}
		else
		{
			self.hitradius(gs.seenineblastradius, 10, 15, 20, 25);
			self.Destroy();
		}
	}
}

