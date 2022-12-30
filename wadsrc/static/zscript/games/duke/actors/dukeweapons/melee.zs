
class DukeMeleeAttack : DukeActor
{
	meta sound attacksound;
	meta int extradamage;
	property attacksound: attacksound;
	property extradamage: extradamage;
	default
	{
		pic "KNEE";
		+DIENOW;
		DukeMeleeAttack.extradamage 0;
		DukeMeleeAttack.attacksound "KICK_HIT";
	}

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sectp = actor.sector;
		double vel = 1024., zvel;
		HitInfo hit;

		if (p)
		{
			[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 16.);
			pos.Z += 6;
			ang += 2.64;
		}
		else
		{
			double x;
			let pactor = self.findplayer().Actor;
			zvel = ((pactor.pos.Z - pos.Z) * 16) / (x + 1 / 16.);
			ang = (pactor.pos.XY - pos.XY).Angle();
		}

		Raze.hitscan(pos, sectp, (ang.ToVector() * vel, zvel * 64), hit, CLIPMASK1);

		if ((ud.mapflags & MFLAG_ALLSECTORTYPES) && hit.hitSector != nullptr && 
			((hit.hitSector.lotag == ST_160_FLOOR_TELEPORT && zvel > 0) || (hit.hitSector.lotag == ST_161_CEILING_TELEPORT && zvel < 0))
			&& hit.hitActor == nullptr && hit.hitWall == nullptr)
		{
			DukeStatIterator its;
			for (let effector = its.First(STAT_EFFECTOR); effector; effector = its.Next())
			{
				if (effector.sector == hit.hitSector && effector.GetClassName() == 'DukeSectorEffector' && effector.ownerActor && effector.lotag == SE_7_TELEPORT)
				{
					let owner = effector.ownerActor;
					Vector3 npos;
					npos.XY = hit.hitpos.XY + (owner.pos.XY - effector.pos.XY);
					if (hit.hitSector.lotag == ST_161_CEILING_TELEPORT)
					{
						npos.Z = owner.sector.floorz;
					}
					else
					{
						npos.Z = owner.sector.ceilingz;
					}
					Raze.hitscan(npos, owner.sector, (ang.ToVector() * 1024, zvel * 0.25), hit, CLIPMASK1);
					break;
				}
			}
		}

		if (hit.hitSector == nullptr) return true;

		if ((pos.XY - hit.hitpos.XY).Sum() < 64)
		{
			if (hit.hitWall != nullptr || hit.hitactor)
			{
				let wpn = dlevel.SpawnActor(hit.hitSector, hit.hitpos, GetClass(), -15, (0, 0), ang, 2., 0., actor, STAT_PROJECTILE);
				if (!wpn) return true;
			
				if (self.extradamage > 0)
				{
					wpn.extra += self.extradamage;
				}
				else
				{
					wpn.extra += random(0, 7);
				}
				if (p)
				{
					let k = wpn.spawn("DukeSmallSmoke");
					if (k) k.pos.Z -= 8;
					wpn.PlayActorSound(self.attacksound);
					if (p.steroids_amount > 0 && p.steroids_amount < 400)
						wpn.extra += (gs.max_player_health >> 2);
				}


				if (hit.hitActor && !Duke.isaccessswitch(hit.hitactor.spritetexture()))
				{
					let da = DukeActor(hit.hitActor);
					da.OnHit(wpn);
					if (p) p.checkhitswitch(nullptr, da);
				}
				else if (hit.hitWall)
				{
					if (hit.hitWall.cstat & CSTAT_WALL_BOTTOM_SWAP)
						if (hit.hitWall.twoSided())
							if (hit.hitpos.Z >= hit.hitWall.nextSectorp().floorz)
								hit.hitWall = hit.hitWall.nextWallp();

					if (!Duke.isaccessswitch(hit.hitWall.walltexture))
					{
						dlevel.checkhitwall(hit.hitWall, wpn, hit.hitpos);
						if (p) p.checkhitswitch(hit.hitWall, nullptr);
					}
				}
			}
			else if (p && zvel > 0 && hit.hitSector.lotag == 1)
			{
				let splash = p.actor.spawn("DukeWaterSplash");
				if (splash)
				{
					splash.pos.XY = hit.hitpos.XY;
					splash.Angle = p.actor.Angle;
					splash.vel.X = 2;
					actor.DoMove(CLIPMASK0);
					splash.vel.X = 0;
				}
			}
		}
		return true;
	}

}

class RedneckBuzzSaw : DukeMeleeAttack
{
	default
	{
		pic "BUZSAW";
	}
}

class RedneckSlingbladeAttack : DukeMeleeAttack
{
	default
	{
		pic "SLINGBLADE";
		DukeMeleeAttack.extradamage 50; // extra attack power.
		DukeMeleeAttack.attacksound "SLINGHIT";
	}
}

