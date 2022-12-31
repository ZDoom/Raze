extend class DukeActor
{
	const KNEE_WEAPON_STRENGTH = 10;
	const PISTOL_WEAPON_STRENGTH = 6;
	const HANDBOMB_WEAPON_STRENGTH = 140;
	const RPG_WEAPON_STRENGTH = 140;
	const SHRINKER_WEAPON_STRENGTH = 0;
	const GROWSPARK_WEAPON_STRENGTH = 15;
	const SHOTGUN_WEAPON_STRENGTH = 10;
	const CHAINGUN_WEAPON_STRENGTH = 9;
	const FREEZETHROWER_WEAPON_STRENGTH = 20;
	const COOL_EXPLOSION_STRENGTH = 38;
	const TRIPBOMB_STRENGTH = 100;
	const FIRELASER_WEAPON_STRENGTH = 7;
	const MORTER_WEAPON_STRENGTH = 50;
	const BOUNCEMINE_WEAPON_STRENGTH = 150;
	const SPIT_WEAPON_STRENGTH = 8;
	const BULLET_WEAPON_STRENGTH = 30;

	const SLINGBLADE_WEAPON_STRENGTH = 50;
	const CASUL_WEAPON_STRENGTH = 20;
	const ALIENGISMO_WEAPON_STRENGTH    =  15;
	const RIFLE_WEAPON_STRENGTH         =  20;
	const PLASMATHROWER_WEAPON_STRENGTH =  10;
	const POWDERKEG_STRENGTH            = 100;
	const FIRELASER_RR_WEAPON_STRENGTH  =  25;
	const SHITBALL_WEAPON_STRENGTH      =   8;
	const BOWLINGBALL_WEAPON_STRENGTH   =   5;
	const THROWSAW_WEAPON_STRENGTH      =  100;
	const BUZSAW_WEAPON_STRENGTH        =  20;



	DukeActor shootprojectile1(DukeActor actor, DukePlayer p, Vector3 pos, double ang, double vel, double zofs_post = 0, double scale = 0) const
	{
		sectortype sect = actor.sector;
		int scount;
		double zvel;

		if (actor.extra >= 0 && !self.bDONTLIGHTSHOOTER) actor.shade = -96; // not for shitball

		if (p != null)
		{
			let aimed = actor.aim(self);

			if (aimed)
			{
				double dal = ((aimed.scale.X * aimed.spriteHeight()) * 0.5);
				double dist = (p.actor.pos.XY - aimed.pos.XY).Length();

				zvel = ((aimed.pos.Z - pos.Z - dal) * vel) / dist;
				ang = (aimed.pos.XY - pos.XY).Angle();
			}
			else
			{
				[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 49.);
			}
		}
		else
		{
			pos += actor.SpecialProjectileOffset();
			
			let j = actor.findplayer();
			
			if (actor.projectilespread < 0)
				ang += frandom(self.projectilespread, 0);
			else
				ang += frandom(-self.projectilespread / 2, self.projectilespread / 2);

			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			zvel = ((j.actor.opos.Z + j.actor.viewzoffset - pos.Z + 3) * vel) / dist;
		}

		if (scale <= 0) scale = p? 0.109375 : 0.28125;

		pos.Z += zofs_post;

		let spawned = dlevel.SpawnActor(sect, pos, self.GetClass(), -127, (scale, scale), ang, vel, zvel, actor, STAT_PROJECTILE);
		if (!spawned) return nullptr;
		spawned.extra += random(0, 7);

		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.clipdist = 1;
		return spawned;
	}
	
	virtual Vector3 SpecialProjectileOffset()
	{
		return (0, 0, 0);
	}
}

class DukeRadiusExplosion : DukeActor
{
	default
	{
		pic "RADIUSEXPLOSION";
		+INFLAME;
		+DIENOW;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
	}
}

