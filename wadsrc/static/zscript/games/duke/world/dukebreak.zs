class DukeBreakWalls
{
	static void breakwall(TextureID newpn, DukeActor spr, walltype wal)
	{
		wal.SetTexture(walltype.main, newpn);
		spr.PlayActorSound("VENT_BUST");
		spr.PlayActorSound("GLASS_HEAVYBREAK");
		spr.lotsofglass(10, wal);
	}
	
	static void ForcefieldHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		sectortype sptr = Raze.updatesector(pos.XY, spr.sector);
		if (sptr == nullptr) return;
		
		double scale = spr.isPlayer()? 0.125 : spr is 'DukeChaingunShot'? 0.25 : 0.5;					
		let spawned = dlevel.SpawnActor(sptr, pos, 'DukeForceRipple', -127, (scale, scale), 0, 0., 0., spr, DukeActor.STAT_MISC);
		if (spawned)
		{
			spawned.cstat |= CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER;
			spawned.angle = wal.delta().Angle() + 90;

			spawned.PlayActorSound(snd);
		}
	}

	static void ForcefieldHitAnim(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		wal.extra = 1;
		ForcefieldHit(wal, newtex, snd, spr, pos);
	}
	
	static void FanSpriteHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		wal.SetTexture(walltype.over, newtex);
		wal.cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
		let nwal = wal.nextwallp();
		if (nwal)
		{
			nwal.SetTexture(walltype.over, newtex);
			nwal.cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
		}
		spr.PlayActorSound(snd);
		spr.PlayActorSound("GLASS_BREAKING");
	}
	
	static void GlassHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		let sptr = Raze.updatesector(pos.XY, spr.sector);
		if (sptr == nullptr) return;
		wal.setTexture(walltype.over, newtex);
		spr.lotsofglass(10, wal);
		wal.cstat = 0;

		let nwal = wal.nextwallp();
		if (nwal) nwal.cstat = 0;

		let spawned = dlevel.SpawnActor(sptr, pos, "DukeSectorEffector", 0, (0, 0), spr.Angle, 0., 0., spr, DukeActor.STAT_EFFECTOR);
		if (spawned)
		{
			spawned.lotag = SE_128_GLASS_BREAKING;
			spawned.temp_data[1] = 5;
			spawned.temp_walls[0] = wal;
			spawned.PlayActorSound(snd);
		}
	}
	
	static void StainGlassHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		let sptr = Raze.updatesector(pos.XY, spr.sector);
		if (sptr == nullptr) return;
		wal.setTexture(walltype.over, newtex);
		spr.lotsofcolourglass(10, wal);
		wal.cstat = 0;

		let nwal = wal.nextwallp();
		if (nwal) nwal.cstat = 0;

		spr.PlayActorSound("VENT_BUST");
		spr.PlayActorSound(snd);
	}

	static void TechStuffBreak(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		breakwall(newtex, spr, wal);
		spr.PlayActorSound(snd);
	}

	static void ScreenBreak(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		static const String randbreak[] = { "W_SCREENBREAK", "W_SCREENBREAK2", "W_SCREENBREAK3" };
		spr.lotsofglass(30, wal);
		wal.SetTextureName(walltype.main, randbreak[random(0, 2)]);
		spr.PlayActorSound(snd);
	}

	static void LightBreak(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		if (Duke.rnd(128))
			spr.PlayActorSound("GLASS_HEAVYBREAK");
		else spr.PlayActorSound("GLASS_BREAKING");
		spr.lotsofglass(30, wal);
		wal.setTexture(walltype.main, newtex);

		if (!wal.lotag) return;
		if (!wal.twoSided()) return;
		int darkestwall = 0;

		let nextsect = wal.nextsectorp();
		foreach (wl : nextsect.walls)
			if (wl.shade > darkestwall)
				darkestwall = wl.shade;

		int j = random(0, 1);
		DukeStatIterator it;
		for(let effector = it.First(DukeActor.STAT_EFFECTOR); effector; effector = it.Next())
		{
			if (effector.hitag == wal.lotag && effector.lotag == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
			{
				effector.temp_data[2] = j;
				effector.temp_data[3] = darkestwall;
				effector.temp_data[4] = 1;
			}
		}
	}
	
	static void ATMBreak(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		wal.setTexture(walltype.main, newtex);
		spr.PlayActorSound(snd);
		spr.lotsofstuff('DukeMoney', random(1, 8));
	}
	
}
