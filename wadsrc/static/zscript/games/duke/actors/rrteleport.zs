
class RedneckTeleportDest : DukeActor
{
	default
	{
		pic "RRTELEPORTDEST";
		statnum STAT_TELEPORT;
		ScaleX 1;
		ScaleY 1;
		clipdist 16;
	}
}

class RedneckTeleport : RedneckTeleportDest
{
	default
	{
		pic "RRTELEPORT";
	}
	
	override void Tick()
	{
		double xx;
		DukePlayer p;
		[p, xx] = self.findplayer();
		if (xx < 128)
		{
			DukeStatIterator it;
			for(let act2 = it.First(STAT_TELEPORT); act2; act2 = it.Next())
			{
				if (act2.GetClassName() == 'RedneckTeleportDest')
				{
					p.setTargetAngle(act2.angle, true);
					let pactor = p.actor;
					pactor.pos = act2.pos.plusZ(-36 + gs.playerheight);
					pactor.backuppos();
					p.setbobpos();
					pactor.ChangeSector(act2.sector);
					p.cursector = pactor.sector;
					act2.PlayActorSound("TELEPORTER");
					act2.cstat = CSTAT_SPRITE_INVISIBLE;
					act2.cstat2 |= CSTAT2_SPRITE_NOFIND;
					act2.ChangeStat(STAT_REMOVED);	// this is still needed for the sound so don't destroy.
				}
			}
		}
	}
}
