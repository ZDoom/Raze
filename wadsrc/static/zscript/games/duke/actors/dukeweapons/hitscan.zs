
extend class DukeActor
{
	static void tracers(Vector3 start, Vector3 dest, int n)
	{
		sectortype sect = nullptr;

		let direction = dest - start;

		if (direction.XY.Sum() < 192.75)
			return;

		let pos = start;
		let add = direction / (n + 1);
		for (int i = n; i > 0; i--)
		{
			pos += add;
			sect = Raze.updatesector(pos.XY, sect);
			if (sect)
			{
				if (sect.lotag == ST_2_UNDERWATER)
				{
					Vector2 scale = (0.0625 + random(0, 3) * REPEAT_SCALE, 0.0625 + random(0, 3) * REPEAT_SCALE);
					dlevel.SpawnActor(sect, pos, "DukeWaterBubble", -32, scale, frandom(0, 360), 0., 0., Duke.GetLocalPlayer().actor, STAT_MISC);
				}
				else
					dlevel.SpawnActor(sect, pos, "DukeSmallSmoke", -32, (0.21875, 0.21875), 0, 0., 0., Duke.GetLocalPlayer().actor, STAT_MISC);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	bool HitscanAttack(DukeActor actor, DukePlayer p, Vector3 pos, double ang, double hspread, double vspread, double enemyspread, bool forcespread, double aimangle = -1, bool waterhalfhitchance = false, class<DukeActor> sparktype = "DukeShotSpark") const
	{
		let sectp = actor.sector;
		double vel = 1024, zvel = 0;
		HitInfo hit;

		if (actor.extra >= 0) actor.shade = -96;

		if (p != null)
		{
			let aimed = actor.aim(self, aimangle);
			if (aimed)
			{
				double dal = ((aimed.scale.X * aimed.spriteHeight()) * 0.5) + aimed.sparkoffset;
				double dist = (p.actor.pos.XY - aimed.pos.XY).Length();
				zvel = ((aimed.pos.Z - pos.Z - dal) * 16) / dist;
				ang = (aimed.pos - pos).Angle();
			}
			
			if (aimed == nullptr || forcespread)
			{
				ang += hspread / 2 - frandom(0, hspread);
				if (aimed == nullptr) [vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 16);
				zvel += vspread / 8 - frandom(0, vspread / 4);
			}
			pos.Z -= 2;
		}
		else
		{
			let j = actor.findplayer();
			pos.Z -= 4;
			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			zvel = ((j.actor.pos.Z + j.actor.viewzoffset - pos.Z) * 16) / dist;
			zvel += frandom(-0.5, 0.5);
			if (!actor.bALTHITSCANDIRECTION)
			{
				ang += enemyspread / 2 - frandom(0, enemyspread);
			}
			else
			{
				// one of those lousy hacks in Duke.
				ang = (j.actor.pos.XY - pos.XY).Angle() + enemyspread - frandom(0, enemyspread * 2);
			}
		}

		actor.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
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

		actor.cstat |= CSTAT_SPRITE_BLOCK_ALL;

		if (hit.hitSector == nullptr) return true;

		// RR shotgun only
		if (waterhalfhitchance && hit.hitSector.lotag == ST_1_ABOVE_WATER && random(0, 1))
			return true;

		if (random(0, 15) == 0 && hit.hitSector.lotag == ST_2_UNDERWATER)
			tracers(hit.hitpos, pos, 8 - (ud.multimode >> 1));

		DukeActor spark = null;
		if (p != null)
		{
			spark = dlevel.SpawnActor(hit.hitSector, hit.hitpos, sparktype, -15, (0.15625, 0.15625), ang, 0., 0., actor, STAT_PROJECTILE);
			if (!spark) return true;

			spark.extra = self.strength + random(0, 5);

			if (hit.hitWall == nullptr && hit.hitActor == nullptr)
			{
				if (zvel < 0)
				{
					if (hit.hitSector.ceilingstat & CSTAT_SECTOR_SKY)
					{
						spark.scale = (0, 0);
						return true;
					}
					else
						dlevel.checkhitceiling(hit.hitSector, spark);
				}
				if (zvel < 0 || hit.hitSector.lotag != ST_1_ABOVE_WATER)
					spark.spawn("DukeSmallSmoke");
			}

			let hitActor = DukeActor(hit.hitActor);
			if (hitActor)
			{
				if (hitActor.bNOHITSCANHIT)
					return true;
				hitActor.OnHit(spark);
				if (hitActor.isPlayer() && (ud.coop != 1 || ud.ffire == 1))
				{
					let jib = spark.spawn("DukeJibs6");
					spark.scale = (0, 0);
					if (jib)
					{
						jib.pos.Z += 4;
						jib.vel.X = 1;
						jib.scale = (0.375, 0.375);
						jib.Angle += frandom(-11.25, 11.25);
					}
				}
				else spark.spawn("DukeSmallSmoke");

				if (p != null && Duke.isshootableswitch(hitActor.spritetexture()))
				{
					p.checkhitswitch(nullptr, hitActor);
					return true;
				}
			}
			else if (hit.hitWall)
			{
				spark.spawn("DukeSmallSmoke");

				if (!(Raze.tileflags(hit.hitWall.walltexture) & (Duke.TFLAG_DOORWALL | Duke.TFLAG_BLOCKDOOR)))
				{
					if (p != null && Duke.isshootableswitch(hit.hitWall.walltexture))
					{
						p.checkhitswitch(hit.hitWall, nullptr);
						return true;
					}

					if (!(hit.hitWall.hitag != 0 || (hit.hitWall.twoSided() && hit.hitWall.nextWallp().hitag != 0)))
					{

						if (hit.hitSector && hit.hitSector.lotag == 0 && !(Raze.tileflags(hit.hitWall.overtexture) & Duke.TFLAG_FORCEFIELD))
						{
							if ((hit.hitWall.twoSided() && hit.hitWall.nextSectorp().lotag == 0) || (!hit.hitWall.twoSided() && hit.hitSector.lotag == 0))
							{
								if ((hit.hitWall.cstat & CSTAT_WALL_MASKED) == 0)
								{
									bool ok = true;
									if (hit.hitWall.twoSided())
									{
										DukeSectIterator it;
										for (let l = it.First(hit.hitWall.nextSectorp()); l; l = it.Next())
										{
											if (l.statnum == STAT_EFFECTOR && l.lotag == SE_13_EXPLOSIVE)
											{
												ok = false;
												break;
											}
										}
									}

									if (ok)
									{
										DukeStatIterator it;
										for (let l = it.First(STAT_MISC); l; l = it.Next())
										{
											if (l is 'DukeBulletHole' && (l.pos - spark.pos).Length() < frandom(0.75, 1.25))
											{
												ok = false;
												break;
											}
										}
										if (ok)
										{
											let hole = spark.spawn("DukeBulletHole");
											if (hole)
											{
												hole.vel.X = -1 / 16.;
												hole.Angle = hit.hitWall.delta().Angle() - 90;
												hole.DoMove(CLIPMASK0);
												hole.cstat2 |= CSTAT2_SPRITE_DECAL;
											}
										}
									}
								}
							}
						}
					}
				}

				if (hit.hitWall.cstat & CSTAT_WALL_BOTTOM_SWAP)
					if (hit.hitWall.twoSided())
						if (hit.hitpos.Z >= hit.hitWall.nextSectorp().floorz)
							hit.hitWall = hit.hitWall.nextWallp();

				dlevel.checkhitwall(hit.hitWall, spark, hit.hitpos);
			}
		}
		else
		{
			spark = dlevel.SpawnActor(hit.hitSector, hit.hitpos, sparktype, -15, (0.375, 0.375), ang, 0., 0., actor, STAT_PROJECTILE);
			if (spark)
			{
				spark.extra = self.strength;
				let hitActor = DukeActor(hit.hitActor);

				if (hitActor)
				{
					hitActor.OnHit(spark);
					if (!hitActor.isPlayer())
						spark.spawn("DukeSmallSmoke");
					else spark.scale = (0, 0);
				}
				else if (hit.hitWall)
					dlevel.checkhitwall(hit.hitWall, spark, hit.hitpos);
			}
		}

		if (spark && random(0, 255) < 4)
		{
			spark.PlayActorSound("PISTOL_RICOCHET");
		}
		return true;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeShotSpark : DukeActor
{
	default
	{
		pic "SHOTSPARK1";
		+FORCERUNCON;
		+LIGHTDAMAGE;
		statnum STAT_MISC;
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 5.625, 4, 11.25, false);
	}
}

class DukeShotgunShot : DukeActor
{
	default
	{
		pic "SHOTGUN";
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 5.625, 4, 11.25, true);
	}
}

class DukeChaingunShot : DukeActor
{
	default
	{
		pic "CHAINGUN";
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 5.625, 4, 11.25, true);
	}
}

// RR'  damage properties are a bit different.
class RedneckShotSpark : DukeShotSpark
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 5.625, 4, 11.25, false);
	}
}


class RedneckShotgunShot : DukeShotgunShot
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 22.5, 4, 11.25, true, -1, true);
	}
}


class RedneckChaingunShot : DukeChaingunShot
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		return HitscanAttack(actor, p, pos, ang, 5.625, 4, 11.25, true);
	}
}

extend class DukeActor
{
	// Todo: once we have real weapons, this should be cleaned up.
	double WW2GIAimAngle(DukePlayer p)
	{
		if (p == null) return -1;
		int lAmount = p.GetGameVar("PLR_MORALE", -1);
		if (lAmount >= 40) return -1;
		return 7.91015625;
	}
	
	double, double WW2GIGetShotRange(DukePlayer p)
	{
		// the logic here looks very broken...
		if (p == null) return 5.625, 4;
		int lAmount = p.GetGameVar("PLR_MORALE", -1);
		
		if (lAmount < 25) return 11.25, 4;
		if (lAmount > 70) switch(p.curr_weapon)
		{
			case DukeWpn.PISTOL_WEAPON:
				return 5.626, 4;
			case DukeWpn.CHAINGUN_WEAPON:
				return 2.8175, 2;
			case DukeWpn.SHRINKER_WEAPON:
				return 11.25, 8;
			default:
				return 2.8175, 4;
		}
		return 5.625, 4;
	}
}
		

// WW2GI has different settings.
class WW2GIShotSpark : DukeShotSpark
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		let [hspread, vspread] = self.WW2GIGetShotRange(p);
		return HitscanAttack(actor, p, pos, ang, hspread, vspread, 11.25, true, self.WW2GIAimAngle(p));
	}
}

class WW2GIShotgunShot : DukeShotgunShot
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		let [hspread, vspread] = self.WW2GIGetShotRange(p);
		return HitscanAttack(actor, p, pos, ang, hspread, vspread, 11.25, true, self.WW2GIAimAngle(p));
	}
}


class WW2GIChaingunShot : DukeChaingunShot
{
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		let [hspread, vspread] = self.WW2GIGetShotRange(p);
		return HitscanAttack(actor, p, pos, ang, hspread, vspread, 11.25, true, self.WW2GIAimAngle(p));
	}
}
