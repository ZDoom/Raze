// Container for handler functions that handle walls with breakable textures.
// Note that this must be a class, not a struct, so that the internal lookup functions can find it.

class RedneckBreakWalls
{
	static void SinglePlayerBreak(walltype wal, TextureID newtex, Sound snd, DukeActor hitter, Vector3 pos)
	{
		if (ud.multimode < 2)
		{
			wal.SetTexture(walltype.main, newtex);
			hitter.PlayActorSound(snd);
		}
	}
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------
	
	private static int sgn(double v)
	{
		return (v > 0)  - (v < 0);
	}

	static void lotsofpopcorn(DukeActor actor, walltype wal, int n)
	{
		let pos = wal.pos;
		let delta = wal.delta() / (n + 1);

		pos.X -= Sgn(delta.X) * maptoworld;
		pos.Y += Sgn(delta.Y) * maptoworld;

		for (int j = n; j > 0; j--)
		{
			pos += delta;
			let sect = Raze.updatesector(pos, actor.sector);
			if (sect)
			{
				double z = sect.floorz - frandom(0, abs(sect.ceilingz - sect.floorz));
				if (abs(z) > 32)
					z = actor.pos.Z - 32 + frandom(0, 64);
				let a = actor.Angle - 180;
				let vel = frandom(2, 6);
				let zvel = -frandom(0, 4);

				dlevel.SpawnActor(actor.sector, (pos, z), 'RedneckPopcorn', -32, (0.5625, 0.5625), a, vel, zvel, actor, DukeActor.STAT_MISC);
			}
		}
	}

	
	static void PopcornHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		let sptr = Raze.updatesector(pos.XY, spr.sector);
		if (sptr == nullptr) return;
		wal.setTexture(walltype.over, newtex);
		lotsofpopcorn(spr, wal, 64);
		wal.cstat = 0;

		let nwal = wal.nextwallp();
		if (nwal) nwal.cstat = 0;

		let spawned = dlevel.SpawnActor(sptr, pos, "DukeSectorEffector", 0, (0, 0), spr.Angle, 0., 0., spr, DukeActor.STAT_EFFECTOR);
		if (spawned)
		{
			spawned.lotag = SE_128_GLASS_BREAKING;
			//spawned.temp_data[1] = 5;
			spawned.temp_walls[0] = wal;
			spawned.PlayActorSound(snd);
		}
	}

	static void PickupHit(walltype wal, TextureID newtex, Sound snd, DukeActor spr, Vector3 pos)
	{
		let sect = wal.sectorp();
		DukeSectIterator it;
		for(let act = it.First(sect); act; act = it.Next())
		{
			if (act.lotag == 6)
			{
				act.spriteextra++;
				if (act.spriteextra == 25)
				{
					foreach(wl : sect.walls)
					{
						let nsec = wl.nextSectorp();
						if (nsec) nsec.lotag = 0;
					}
					sect.lotag = 0;
					act.StopSound(-1);
					act.PlayActorSound(snd);
					act.ChangeStat(DukeActor.STAT_REMOVED);
					act.cstat2 |= CSTAT2_SPRITE_NOFIND;
					act.lotag = 0;
				}
			}
		}
	}

}
