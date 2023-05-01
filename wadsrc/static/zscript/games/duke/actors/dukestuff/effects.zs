
class DukeToiletWater : DukeActor
{
	default
	{
		pic "TOILETWATER";
		+FORCERUNCON;
		+NOTELEPORT;
		Strength 0;
		action "TOILETWATERFRAMES", 0, 4, 1, 1, 1;
		StartAction "TOILETWATERFRAMES";
	}
	
	override void Initialize()
	{
		self.shade = -16;
		self.changeStat(STAT_STANDABLE);
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (pdist < 8192 * maptoworld)
		{
			self.PlayActorSound("WATER_GURGLE", CHAN_AUTO, CHANF_SINGULAR);
			if (self.attackertype.GetClassName() == 'DukeToilet')
			{
				self.actorsizeto(34 * REPEAT_SCALE, 34 * REPEAT_SCALE);
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeWaterFountainBroke')
				{
					self.actorsizeto(6 * REPEAT_SCALE, 15 * REPEAT_SCALE);
				}
				else if (self.attackertype.GetClassName() != 'DukeToiletWater')
				{
					self.actorsizeto(24 * REPEAT_SCALE, 32 * REPEAT_SCALE);
				}
			}
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.checkp(p, pfacing))
					{
						if (self.actioncounter >= 32)
						{
							if (p.actor.extra < MAXPLAYERHEALTH)
							{
								if (p.PlayerInput(Duke.SB_OPEN))
								{
									if (self.cansee(p))
									{
										p.addphealth(1, false);
										p.actor.PlayActorSound("PLAYER_DRINKING", CHAN_AUTO, CHANF_LOCAL);
										self.actioncounter = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
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
		action "EXPLOSION_FRAMES", 0, 20, 1, 1, 4;
		action "EXPLOSION_FRAMES_FAST", 1, 19, 1, 1, 2;
		startAction "EXPLOSION_FRAMES";
	}
	
	override bool animate(tspritetype t)
	{
		Duke.GetViewPlayer().visibility = -127;
		Duke.setlastvisinc(32);
		return false;
	}

	override void Initialize()
	{
		let owner = self.ownerActor;

		if (owner && owner != self)
		{
			self.Angle = owner.Angle;
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'EXPLOSION_FRAMES')
		{
			if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				setAction('EXPLOSION_FRAMES_FAST');
				self.Scale = (38 * REPEAT_SCALE, 38 * REPEAT_SCALE);
			}
			if (self.actioncounter >= 20)
			{
				self.killit();
			}
		}
		else
		{
			if (self.actioncounter >= 19)
			{
				self.killit();
			}
		}
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
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 20)
		{
			self.killit();
		}
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
		action "EXPLOSION3_FRAMES", -1, 20, 1, 1, 4;
		StartAction "EXPLOSION3_FRAMES";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		self.scale = (2, 2);
	}
	
	override bool animate(tspritetype t)
	{
		return false;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 20)
		{
			self.killit();
		}
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
		action "FFLAME_FR", 0, 16, 1, 1, 1;
		action "FFLAME", 0, 1, 1, 1, 1;
		StartAction "FFLAME_FR";
	}

	override void Initialize()
	{
		self.shade = -127;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'FFLAME_FR')
		{
			if (pdist < 1024 * maptoworld)
			{
				self.hitradius(1024, WEAKEST, WEAKEST, WEAKEST, WEAKEST);
			}
			if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			if (self.actioncounter >= 16)
			{
				setAction('FFLAME');
			}
		}
		if (self.curAction.name == 'FFLAME')
		{
			if (Duke.rnd(4))
			{
				setAction('FFLAME_FR');
				self.PlayActorSound("CAT_FIRE");
				self.actioncounter = 0;
			}
		}
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
		action "RIP_F", 0, 8, 1, 1, 1;
		Strength 0;
		//StartAction "RIP_F";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.actioncounter >= 8)
		{
			self.killit();
		}
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
		action "TRANSFOWARD", 0, 6, 1, 1, 2;
		action "TRANSBACK", 5, 6, 1, -1, 2;
		Strength 0;
		//StartAction "TRANSFOWARD";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner == nullptr || owner == self) 
		{
			scale = (0, 0);
			return;
		}
		if (owner.statnum == STAT_PROJECTILE)
		{
			self.scale = (0.125, 0.125);
		}
		else
		{
			self.scale = (0.75, 1);
			if (owner.statnum == STAT_PLAYER || owner.badguy())
				self.pos.Z -= 32;
		}

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = owner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'TRANSFOWARD')
		{
			if (self.actioncounter >= 6)
			{
				setAction('TRANSBACK');
			}
		}
		else
		{
			if (self.actioncounter >= 6)
			{
				self.killit();
			}
		}
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
		action "BEAMFOWARD", 0, 4, 1, 1, 9;
		Strength 0;
		StartAction "BEAMFOWARD";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner == nullptr || owner == self) 
		{
			scale = (0, 0);
			return;
		}
		self.scale = (0.484375, REPEAT_SCALE);
		self.pos.Z = owner.sector.floorz - gs.playerheight;

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = owner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		self.actorsizeto(32 * REPEAT_SCALE, 64 * REPEAT_SCALE);
		self.actorsizeto(32 * REPEAT_SCALE, 64 * REPEAT_SCALE);
		self.actorsizeto(32 * REPEAT_SCALE, 64 * REPEAT_SCALE);
		if (self.actioncounter >= 4)
		{
			self.killit();
		}
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
		action "STEAMFRAMES", 0, 5, 1, 1, 1;
		Strength 0;
		StartAction "STEAMFRAMES";
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (pdist < 8192 * maptoworld)
		{
			self.PlayActorSound("STEAM_HISSING", CHAN_AUTO, CHANF_SINGULAR);
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.counter >= 20)
		{
			self.counter = 0;
			if (self.checkp(p, palive))
			{
				if (pdist < 1024 * maptoworld)
				{
					p.addphealth(-1, self.bBIGHEALTH);
					p.pals = color(16, 16, 0, 0);
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
		else
		{
			if (self.attackertype is 'DukeSteamBase')
			{
				return;
			}
			self.actorsizeto(24 * REPEAT_SCALE, 24 * REPEAT_SCALE);
		}
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
	
	override void Initialize()
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
		move "SMOKEVEL", 8, -16;
		move "ENGINE_SMOKE", 64, -64;
		move "SMOKESHOOTOUT", -192;
		action "SMOKEFRAMES", 0, 4, 1, 1, 10;
		Strength 0;
		StartAction "SMOKEFRAMES";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;

		if (owner && owner != self)
		{
			self.Angle = owner.Angle;
			self.cstat = randomXFlip();

			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.scale = (0.375, 0.375);
		self.ChangeStat(STAT_MISC);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curMove.name == 'none')
		{
			if (self.attackertype.GetClassName() == 'DukeRecon')
			{
				setMove('SMOKESHOOTOUT', geth);
			}
			else if (self.attackertype.GetClassName() == 'DukeSectorEffector')
			{
				setMove('ENGINE_SMOKE', geth | getv);
			}
			else
			{
				setMove('SMOKEVEL', geth | getv | faceplayer);
			}
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.cstat = CSTAT_SPRITE_TRANSLUCENT;
			}
			else if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
			{
				self.Scale = (80 * REPEAT_SCALE, 80 * REPEAT_SCALE);
			}
			else
			{
				if (self.attackertype.GetClassName() == 'DukeOnFireSmoke')
				{
					self.Scale = (40 * REPEAT_SCALE, 40 * REPEAT_SCALE);
					self.cstat |= CSTAT_SPRITE_TRANSLUCENT;
				}
			}
		}
		if (self.attackertype.GetClassName() == 'DukeFlamethrowerFlame')
		{
			self.actorsizeto(128 * REPEAT_SCALE, 128 * REPEAT_SCALE);
		}
		if (pdist < 1596 * maptoworld)
		{
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				self.killit();
			}
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		if (self.actioncounter >= 4)
		{
			self.killit();
		}
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
		action "BLOODFRAMES", 0, 4, 1, 1, 15;
		Strength 0;
		StartAction "BLOODFRAMES";
	}
	
	override void Initialize()
	{
		self.pos.Z -= 26;
		if (!mapSpawned && self.ownerActor && self.ownerActor.pal == 6)
			self.pal = 6;
		self.scale = (0.25, 0.25);
		temp_pos.Z = 72 * REPEAT_SCALE;
		self.ChangeStat(STAT_MISC);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		let z = temp_pos.Z;
		self.actorsizeto(z, z);
		self.actorsizeto(z, z);
		self.actorsizeto(z, z);
		if (pdist > 3144 * maptoworld || self.actioncounter >= 4)
		{
			self.killit();
		}
	}
}

class RedneckBlood : DukeBlood
{
	override void Initialize()
	{
		Super.Initialize();
		self.scale = (0.0625, 0.0625);
		temp_pos.Z = 48 * REPEAT_SCALE;
	}
}
		

