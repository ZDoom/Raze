
class DukeToiletWater : DukeActor
{
	default
	{
		pic "TOILETWATER";
		+FORCERUNCON;
		+NOTELEPORT;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.shade = -16;
		self.changeStat(STAT_STANDABLE);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeExplosion2 : DukeActor
{
	default
	{
		pic "EXPLOSION2";
		+FULLBRIGHT;
		+FORCERUNCON;
		+TRIGGER_IFHITSECTOR;
		Strength 1;
	}
	
	override bool animate(tspritetype t)
	{
		Duke.GetViewPlayer().visibility = -127;
		Duke.setlastvisinc(32);
		return false;
	}

	override void Initialize(DukeActor spawner)
	{
		if (spawner && spawner != self)
		{
			self.Angle = spawner.Angle;
			self.cstat = randomXFlip();

			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.cstat |= CSTAT_SPRITE_YCENTER;
		self.shade = -127;
		self.Scale = (0.75, 0.75);
		self.ChangeStat(STAT_MISC);
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeExplosion2Bot : DukeExplosion2
{
	default
	{
		pic "EXPLOSION2BOT";
		-TRIGGER_IFHITSECTOR;
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RedneckExplosion3 : DukeExplosion2
{
	default
	{
		pic "EXPLOSION3";
	}
	
	override void Initialize(DukeActor spawner)
	{
		Super.Initialize(spawner);
		self.scale = (2, 2);
	}
	
	override bool animate(tspritetype t)
	{
		return false;
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeFloorFlame : DukeActor
{
	default
	{
		pic "FLOORFLAME";
		+FULLBRIGHT;
		+FORCERUNCON;
		Strength 0;
	}

	override void Initialize(DukeActor spawner)
	{
		self.shade = -127;
		self.ChangeStat(STAT_STANDABLE);
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeForceRipple : DukeActor
{
	default
	{
		pic "FORCERIPPLE";
		+FORCERUNCON;
		Strength 0;
		//StartAction "RIP_F";
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeTransporterStar : DukeActor
{
	default
	{
		pic "TRANSPORTERSTAR";
		+FORCERUNCON;
		+NOTELEPORT;
		Strength 0;
		//StartAction "TRANSFOWARD";
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner == nullptr || spawner == self) 
		{
			scale = (0, 0);
			return;
		}
		if (spawner.statnum == STAT_PROJECTILE)
		{
			self.scale = (0.125, 0.125);
		}
		else
		{
			self.scale = (0.75, 1);
			if (spawner.statnum == STAT_PLAYER || spawner.badguy())
				self.pos.Z -= 32;
		}

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = spawner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeTransporterBeam : DukeActor
{
	default
	{
		pic "TRANSPORTERBEAM";
		+FORCERUNCON;
		+NOTELEPORT;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner == nullptr || spawner == self) 
		{
			scale = (0, 0);
			return;
		}
		self.scale = (0.484375, REPEAT_SCALE);
		self.pos.Z = spawner.sector.floorz - gs.playerheight;

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = spawner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeSteamBase : DukeActor // we need this for in-game checking and the shared CON code.
{
	default
	{
		statnum STAT_STANDABLE;
		+FORCERUNCON;
		Strength 0;
	}
	

}

class DukeCeilingSteam : DukeSteamBase
{
	default
	{
		pic "CEILINGSTEAM";
	}
}


class DukeSteam : DukeSteamBase
{
	default
	{
		pic "STEAM";
	}
	
	override void Initialize(DukeActor spawner)
	{
		let owner = self.ownerActor;

		if (owner && owner != self)
		{
			self.Angle = owner.Angle;
			self.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
			self.vel.X = -0.5;
			self.doMove(CLIPMASK0);
		}
		self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.ChangeStat(STAT_STANDABLE);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeSmallSmoke : DukeActor
{
	default
	{
		pic "SMALLSMOKE";
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner && spawner != self)
		{
			self.Angle = spawner.Angle;
			self.cstat = randomXFlip();

			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.scale = (0.375, 0.375);
		self.ChangeStat(STAT_MISC);
	}
	
	
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeBlood : DukeActor
{
	default
	{
		pic "BLOOD";
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.pos.Z -= 26;
		if (spawner && spawner.pal == 6)
			self.pal = 6;
		self.scale = (0.25, 0.25);
		temp_pos.Z = 72 * REPEAT_SCALE;
		self.ChangeStat(STAT_MISC);
	}
}

class RedneckBlood : DukeBlood
{
	override void Initialize(DukeActor spawner)
	{
		Super.Initialize(spawner);
		self.scale = (0.0625, 0.0625);
		temp_pos.Z = 48 * REPEAT_SCALE;
	}
}
		

