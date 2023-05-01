class DukeItemBase : DukeActor
{
	const PISTOLAMMOAMOUNT        = 12;
	const SHOTGUNAMMOAMOUNT       = 10;
	const CHAINGUNAMMOAMOUNT      = 50;
	const RPGAMMOBOX              = 5;
	const CRYSTALAMMOAMOUNT       = 5;
	const GROWCRYSTALAMMOAMOUNT   = 20;
	const DEVISTATORAMMOAMOUNT    = 15;
	const FREEZEAMMOAMOUNT        = 25;
	const FLAMETHROWERAMMOAMOUNT  = 25;
	const HANDBOMBBOX             = 5;
	const PIG_SHIELD_AMOUNT1      = 75;
	const PIG_SHIELD_AMOUNT2      = 50;
	const MAXPLAYERATOMICHEALTH   = 200;



	override void Initialize()
	{
		commonItemSetup();
	}	
	
	void setup_respawn()
	{
		setMove('RESPAWN_ACTOR_FLAG', 0);
		if (self.sector != null) 
		{
			if (!isRR()) self.spawn('DukeRespawnMarker');
			else self.spawn('RedneckRespawnMarker');
		}
		self.cstat = CSTAT_SPRITE_INVISIBLE;
	}
	
	void state_respawnit(DukePlayer p, double pdist)
	{
		if (self.counter >= RESPAWNITEMTIME)
		{
			if (self.sector != null) self.spawn('DukeTransporterStar');
			setMove('none', 0);
			self.cstat = 0;
			self.PlayActorSound("TELEPORTER");
		}
	}
	
	void state_getcode(DukePlayer p, double pdist, Sound snd = "PLAYER_GET")
	{
		p.actor.PlayActorSound(snd, CHAN_AUTO, CHANF_LOCAL);
		p.pals = color(16, 0, 32, 0);
		if ((bInventory && ud.respawn_inventory) || (!bInventory && ud.respawn_items))
		{
			setup_respawn();
		}
		else
		{
			self.killit();
		}
	}
	
	void state_quikget(DukePlayer p, double pdist, Sound snd = "PLAYER_GET")
	{
		p.actor.PlayActorSound(snd, CHAN_AUTO, CHANF_LOCAL);
		p.pals = color(16, 0, 32, 0);
		self.killit();
	}
	
	void state_randgetweapsnds(DukePlayer p, double pdist)
	{
		if (Duke.rnd(64))
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON1", CHAN_AUTO, CHANF_LOCAL);
		}
		else if (Duke.rnd(96))
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON2", CHAN_AUTO, CHANF_LOCAL);
		}
		else if (Duke.rnd(128))
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON3", CHAN_AUTO, CHANF_LOCAL);
		}
		else if (Duke.rnd(140))
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON4", CHAN_AUTO, CHANF_LOCAL);
		}
		else if (Sound("PLAYER_GETWEAPON5") == 0 || Duke.Rnd(128)) // RR has one sound more - this code is a compile time check.
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON6", CHAN_AUTO, CHANF_LOCAL);
		}
		else
		{
			p.actor.PlayActorSound("PLAYER_GETWEAPON5", CHAN_AUTO, CHANF_LOCAL);
		}
	}
	void state_getweaponcode(DukePlayer p, double pdist)
	{
		state_randgetweapsnds(p, pdist);
		p.pals = color(32, 0, 32, 0);
		if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !1))
		{
			return;
		}
		if (ud.respawn_items)
		{
			setup_respawn();
		}
		else
		{
			self.killit();
		}
	}
	void state_quikweaponget(DukePlayer p, double pdist)
	{
		state_randgetweapsnds(p, pdist);
		p.pals = color(32, 0, 32, 0);
		if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !1))
		{
			return;
		}
		self.killit();
	}
	
}



class DukeSteroids : DukeItemBase
{
	default
	{
		pic "STEROIDS";
		+INVENTORY;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_STEROIDS, STEROID_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_STEROIDS, STEROID_AMOUNT);
								p.FTA(37);
								if (self.attackertype.GetClassName() == 'DukeSteroids')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeBoots : DukeItemBase
{
	default
	{
		pic "BOOTS";
		+INVENTORY;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_BOOTS, BOOT_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_BOOTS, BOOT_AMOUNT);
								p.FTA(6);
								if (self.attackertype.GetClassName() == 'DukeBoots')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeHeatSensor : DukeItemBase
{
	default
	{
		pic "HEATSENSOR";
		+INVENTORY;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_HEATS, HEAT_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_HEATS, HEAT_AMOUNT);
								p.FTA(101);
								if (self.attackertype.GetClassName() == 'DukeHeatSensor')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeShield : DukeItemBase
{
	default
	{
		pic "SHIELD";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_SHIELD, SHIELD_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								if (self.attackertype.GetClassName() == 'DukePigCop')
								{
									if (Duke.rnd(128))
									{
										p.playeraddinventory(self, GET_SHIELD, PIG_SHIELD_AMOUNT1);
									}
									else
									{
										p.playeraddinventory(self, GET_SHIELD, PIG_SHIELD_AMOUNT2);
									}
									p.FTA(104);
									self.PlayActorSound("KICK_HIT");
									p.pals = color(24, 0, 32, 0);
									self.killit();
								}
								else
								{
									p.playeraddinventory(self, GET_SHIELD, SHIELD_AMOUNT);
								}
								p.FTA(38);
								if (self.attackertype.GetClassName() == 'DukeShield')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeAirtank : DukeItemBase
{
	default
	{
		pic "AIRTANK";
		+INVENTORY;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_SCUBA, SCUBA_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_SCUBA, SCUBA_AMOUNT);
								p.FTA(39);
								if (self.attackertype.GetClassName() == 'DukeAirtank')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeHoloDuke : DukeItemBase
{
	default
	{
		pic "HOLODUKE";
		+INVENTORY;
		Strength 0;
		action "HOLODUKE_FRAMES", 0, 4, 1, 1, 8;
		StartAction "HOLODUKE_FRAMES";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (p.playercheckinventory(self, GET_HOLODUKE, HOLODUKE_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_HOLODUKE, HOLODUKE_AMOUNT);
								p.FTA(51);
								if (self.attackertype.GetClassName() == 'DukeHoloDuke')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeJetpack : DukeItemBase
{
	default
	{
		pic "JETPACK";
		+INVENTORY;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.playercheckinventory(self, GET_JETPACK, JETPACK_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_JETPACK, JETPACK_AMOUNT);
								p.FTA(41);
								if (self.attackertype.GetClassName() == 'DukeJetpack')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeAccessCard : DukeItemBase
{
	default
	{
		pic "ACCESSCARD";
	}
	
	override void Initialize()
	{
		if (ud.multimode > 1 && ud.coop != 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}

		self.scale = (0.5, 0.5);
		self.shade = -17;
		if (!self.mapSpawned) self.ChangeStat(STAT_ACTOR);
		else
		{
			self.ChangeStat(STAT_ZOMBIEACTOR);
			self.makeitfall();
		}
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (p.playercheckinventory(self, GET_ACCESS, 0))
							{
								return;
							}
							p.playeraddinventory(self, GET_ACCESS, 1);
							p.FTA(43);
							state_getcode(p, pdist);
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeAmmo : DukeItemBase
{
	default
	{
		pic "AMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup((0.25, 0.25));
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.PISTOL_WEAPON, PISTOLAMMOAMOUNT)) return;
							p.FTA(65);
							if (self.attackertype.GetClassName() == 'DukeAmmo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeFreezeammo : DukeItemBase
{
	default
	{
		pic "FREEZEAMMO";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.FREEZE_WEAPON, FREEZEAMMOAMOUNT)) return;
							p.FTA(66);
							if (self.attackertype.GetClassName() == 'DukeFreezeammo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeShotgunammo : DukeItemBase
{
	default
	{
		pic "SHOTGUNAMMO";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.SHOTGUN_WEAPON, SHOTGUNAMMOAMOUNT)) return;
							p.FTA(69);
							if (self.attackertype.GetClassName() == 'DukeShotgunammo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeAmmoLots : DukeItemBase
{
	default
	{
		pic "AMMOLOTS";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (self.counter >= 6)
				{
					if (pdist < RETRIEVEDISTANCE * maptoworld)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.PISTOL_WEAPON, 48)) return;
							p.FTA(65);
							if (self.attackertype.GetClassName() == 'DukeAmmoLots')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
					if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
				}
			}
		}
	}
}

class DukeCrystalAmmo : DukeItemBase
{
	default
	{
		pic "CRYSTALAMMO";
		+NOFLOORPAL;
	}
	
	override bool animate(tspritetype t)
	{
		t.shade = int(Raze.BobVal(PlayClock << 4) * 16);
		return false;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.SHRINKER_WEAPON, CRYSTALAMMOAMOUNT)) return;
							p.FTA(78);
							if (self.attackertype.GetClassName() == 'DukeCrystalAmmo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeGrowammo : DukeItemBase
{
	default
	{
		pic "GROWAMMO";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.GROW_WEAPON, GROWCRYSTALAMMOAMOUNT)) return;
							p.FTA(123);
							if (self.attackertype.GetClassName() == 'DukeGrowammo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeBatteryAmmo : DukeItemBase
{
	default
	{
		pic "BATTERYAMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup();
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.CHAINGUN_WEAPON, CHAINGUNAMMOAMOUNT)) return;
							p.FTA(63);
							if (self.attackertype.GetClassName() == 'DukeBatteryAmmo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeDevastatorammo : DukeItemBase
{
	default
	{
		pic "DEVISTATORAMMO";
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.DEVISTATOR_WEAPON, DEVISTATORAMMOAMOUNT)) return;
							p.FTA(14);
							if (self.attackertype.GetClassName() == 'DukeDevastatorammo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeRPGammo : DukeItemBase
{
	default
	{
		pic "RPGAMMO";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (!p.playeraddammo(DukeWpn.RPG_WEAPON, RPGAMMOBOX)) return;
							p.FTA(64);
							if (self.attackertype.GetClassName() == 'DukeRPGammo')
							{
								state_getcode(p, pdist);
							}
							else
							{
								state_quikget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeHBombammo : DukeItemBase
{
	default
	{
		pic "HBOMBAMMO";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.HANDBOMB_WEAPON, HANDBOMBBOX)) return;
							p.FTA(55);
							if (self.attackertype.GetClassName() == 'DukeHBombammo')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeRPGSprite : DukeItemBase
{
	default
	{
		pic "RPGSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.RPG_WEAPON, RPGAMMOBOX)) return;
							p.FTA(56);
							if (self.attackertype.GetClassName() == 'DukeRPGSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeShotgunSprite : DukeItemBase
{
	default
	{
		pic "SHOTGUNSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (self.attackertype.GetClassName() == 'DukePigCop')
							{
								if (!p.playeraddweapon(DukeWpn.SHOTGUN_WEAPON, 0)) return;
								if (Duke.rnd(64))
								{
									if (!p.playeraddammo(DukeWpn.SHOTGUN_WEAPON, 4)) return;
								}
								else if (Duke.rnd(64))
								{
									if (!p.playeraddammo(DukeWpn.SHOTGUN_WEAPON, 3)) return;
								}
								else if (Duke.rnd(64))
								{
									if (!p.playeraddammo(DukeWpn.SHOTGUN_WEAPON, 2)) return;
								}
								else
								{
									if (!p.playeraddammo(DukeWpn.SHOTGUN_WEAPON, 1)) return;
								}
							}
							else
							{
								if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
								{
									return;
								}
								if (!p.playeraddweapon(DukeWpn.SHOTGUN_WEAPON, SHOTGUNAMMOAMOUNT)) return;
								p.FTA(57);
							}
							if (self.attackertype.GetClassName() == 'DukeShotgunSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeSixpak : DukeItemBase
{
	default
	{
		pic "SIXPAK";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.actor.extra < MAXPLAYERHEALTH)
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.addphealth(30, self.bBIGHEALTH);
								p.FTA(62);
								if (self.attackertype.GetClassName() == 'DukeSixpak')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeCola : DukeItemBase
{
	default
	{
		pic "COLA";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.actor.extra < MAXPLAYERHEALTH)
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.addphealth(10, self.bBIGHEALTH);
								p.FTA(61);
								if (self.attackertype.GetClassName() == 'DukeCola')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeAtomicHealth : DukeItemBase
{
	default
	{
		pic "ATOMICHEALTH";
		+FULLBRIGHT;
		+BIGHEALTH;
		+NOFLOORPAL;
	}
	
	override void Initialize()
	{
		commonItemSetup();
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}
	
	override bool animate(tspritetype t)
	{
		t.pos.Z -= 4;
		return false;
	}

	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.actor.extra < MAXPLAYERATOMICHEALTH)
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.addphealth(50, self.bBIGHEALTH);
								p.FTA(19);
								if (self.attackertype.GetClassName() == 'DukeAtomicHealth')
								{
									state_getcode(p, pdist, "GETATOMICHEALTH");
								}
								else
								{
									state_quikget(p, pdist, "GETATOMICHEALTH");
								}
								
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeFirstAid : DukeItemBase
{
	default
	{
		pic "FIRSTAID";
		+INVENTORY;
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (p.playercheckinventory(self, GET_FIRSTAID, FIRSTAID_AMOUNT))
						{
							if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
							{
								p.playeraddinventory(self, GET_FIRSTAID, FIRSTAID_AMOUNT);
								p.FTA(3);
								if (self.attackertype.GetClassName() == 'DukeFirstAid')
								{
									state_getcode(p, pdist);
								}
								else
								{
									state_quikget(p, pdist);
								}
								
							}
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeFirstgunSprite : DukeItemBase
{
	default
	{
		pic "FIRSTGUNSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.PISTOL_WEAPON, 48)) return;
							if (self.attackertype.GetClassName() == 'DukeFirstgunSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeTripbombSprite : DukeItemBase
{
	default
	{
		pic "TRIPBOMBSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.TRIPBOMB_WEAPON, 1)) return;
							p.FTA(58);
							if (self.attackertype.GetClassName() == 'DukeTripbombSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeChaingunSprite : DukeItemBase
{
	default
	{
		pic "CHAINGUNSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.CHAINGUN_WEAPON, 50)) return;
							p.FTA(54);
							if (self.attackertype.GetClassName() == 'DukeChaingunSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeShrinkerSprite : DukeItemBase
{
	default
	{
		pic "SHRINKERSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.SHRINKER_WEAPON, 10)) return;
							p.FTA(60);
							if (self.attackertype.GetClassName() == 'DukeShrinkerSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeFreezeSprite : DukeItemBase
{
	default
	{
		pic "FREEZESPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.FREEZE_WEAPON, FREEZEAMMOAMOUNT)) return;
							p.FTA(59);
							if (self.attackertype.GetClassName() == 'DukeFreezeSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}

class DukeDevastatorSprite : DukeItemBase
{
	default
	{
		pic "DEVISTATORSPRITE";
	}
	override void RunState(DukePlayer p, double pdist)
	{
		self.xoffset = self.yoffset = 0;
		self.fall(p);
		if (self.curMove.name == 'RESPAWN_ACTOR_FLAG')
		{
			state_respawnit(p, pdist);
		}
		else if (!self.checkp(p, pshrunk))
		{
			if (self.checkp(p, palive))
			{
				if (pdist < RETRIEVEDISTANCE * maptoworld)
				{
					if (self.counter >= 6)
					{
						if (Raze.cansee(self.pos.plusZ(random(0, 41)), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.actor.sector))
						{
							if (ud.coop >= 1 && ud.multimode > 1 && p.CheckWeapRec(self, !0))
							{
								return;
							}
							if (!p.playeraddweapon(DukeWpn.DEVISTATOR_WEAPON, DEVISTATORAMMOAMOUNT)) return;
							p.FTA(87);
							if (self.attackertype.GetClassName() == 'DukeDevastatorSprite')
							{
								state_getweaponcode(p, pdist);
							}
							else
							{
								state_quikweaponget(p, pdist);
							}
							
						}
					}
				}
				if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
			}
		}
	}
}
