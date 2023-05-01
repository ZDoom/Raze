class DukeDrone : DukeActor
{
	const DRONESTRENGTH = 150;
	
	default
	{
		pic "DRONE";
		Strength DRONESTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		+FLOATING;
		+QUICKALTERANG;
		+NOJIBS;
		+NOHITJIBS;
		+NOSHOTGUNBLOOD;
		falladjustz 0;
		floating_floordist 30;
		floating_ceilingdist 50;
		
		action "DRONEFRAMES", 0, 1, 7, 1, 1;
		action "DRONESCREAM", 0, 1, 7, 1, 1;
		move "DRONERUNVELS", 128, 64;
		move "DRONERUNUPVELS", 128, -64;
		move "DRONEBULLVELS", 252, -64;
		move "DRONEBACKWARDS", -64, -64;
		move "DRONERISE", 32, -32;
		move "DRONESTOPPED", -16;
		ai "AIDRONEGETE", "DRONESCREAM", "DRONERUNVELS", faceplayerslow| getv;
		ai "AIDRONEWAIT", "DRONEFRAMES", "DRONESTOPPED", faceplayerslow;
		ai "AIDRONEGETUP", "DRONESCREAM", "DRONERUNUPVELS", faceplayer| getv;
		ai "AIDRONEPULLBACK", "DRONEFRAMES", "DRONEBACKWARDS", faceplayerslow;
		ai "AIDRONEHIT", "DRONESCREAM", "DRONEBACKWARDS", faceplayer;
		ai "AIDRONESHRUNK", "DRONEFRAMES", "SHRUNKVELS", fleeenemy;
		ai "AIDRONEDODGE", "DRONEFRAMES", "DRONEBULLVELS", dodgebullet| geth;
		ai "AIDRONEDODGEUP", "DRONEFRAMES", "DRONERISE", getv| geth;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("DRON_RECOG");
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_dronedead(DukePlayer p, double pdist)
	{
		self.addkill();
		self.spawndebris(DukeScrap.Scrap1, 8);
		self.spawndebris(DukeScrap.Scrap2, 4);
		self.spawndebris(DukeScrap.Scrap3, 7);
		if (self.sector != null) self.spawn('DukeExplosion2');
		self.PlayActorSound("RPG_EXPLODE");
		self.hitradius(2048, 15, 20, 25, 30);
		self.killit();
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkdronehitstate(DukePlayer p, double pdist)
	{
		if (self.extra < 0)
		{
			state_dronedead(p, pdist);
		}
		else if (self.ifsquished(p))
		{
			state_dronedead(p, pdist);
		}
		else
		{
			self.PlayActorSound("DRON_PAIN");
			if (self.dodge() == 1)
			{
				if (self.pos.Z - self.ceilingz < 64)
				{
					if (Duke.rnd(48))
					{
						setAI('AIDRONEDODGE');
					}
				}
				setAI('AIDRONEDODGEUP');
			}
			else
			{
				setAI('AIDRONEGETE');
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_droneshrunkstate(DukePlayer p, double pdist)
	{
		if (self.counter >= 24)
		{
			self.killit();
		}
		else
		{
			self.actorsizeto(1 * REPEAT_SCALE, 1 * REPEAT_SCALE);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_checkdronenearplayer(DukePlayer p, double pdist)
	{
		if (self.checkp(p, palive))
		{
			if (pdist < 1596 * maptoworld)
			{
				if (self.counter >= 8)
				{
					self.addkill();
					self.PlayActorSound("DRON_ATTACK2");
					self.spawndebris(DukeScrap.Scrap1, 8);
					self.spawndebris(DukeScrap.Scrap2, 4);
					self.spawndebris(DukeScrap.Scrap3, 7);
					if (self.sector != null) self.spawn('DukeExplosion2');
					self.PlayActorSound("RPG_EXPLODE");
					self.hitradius(2048, 15, 20, 25, 30);
					self.killit();
				}
				else if (self.counter < 3)
				{
					self.PlayActorSound("LASERTRIP_ARMING");
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

	void state_dronegetstate(DukePlayer p, double pdist)
	{
		if (Duke.rnd(192))
		{
			if (self.cansee(p))
			{
				if (self.dodge() == 1)
				{
					setAI('AIDRONEDODGE');
					return;
				}
				if (self.curMove.name == 'DRONEBULLVELS')
				{
					if (self.counter >= 64)
					{
						setAI('AIDRONEPULLBACK');
					}
					else
					{
						if (self.movflag > kHitSector)
						{
							if (self.counter >= 16)
							{
								setAI('AIDRONEPULLBACK');
							}
						}
					}
				}
				else
				{
					if (self.counter >= 32)
					{
						if (self.checkp(p, phigher))
						{
							setMove('DRONEBULLVELS', geth | getv);
						}
						else
						{
							setMove('DRONEBULLVELS', geth);
						}
					}
				}
			}
			else
			{
				if (Duke.rnd(1))
				{
					self.actoroperate();
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void state_dronedodgestate(DukePlayer p, double pdist)
	{
		if (self.curAI == 'AIDRONEDODGEUP')
		{
			if (self.counter >= 8)
			{
				setAI('AIDRONEGETE');
			}
			else
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIDRONEGETE');
				}
			}
		}
		else
		{
			if (self.counter >= 8)
			{
				setAI('AIDRONEGETE');
			}
			else
			{
				if (self.movflag > kHitSector)
				{
					setAI('AIDRONEGETE');
				}
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void RunState(DukePlayer p, double pdist)
	{
		state_checkdronenearplayer(p, pdist);
		if (Duke.rnd(2))
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
		}
		else
		{
			self.PlayActorSound("DRON_JETSND", CHAN_AUTO, CHANF_SINGULAR);
		}
		if (self.curAction.name == 'none')
		{
			setAI('AIDRONEGETE');
		}
		else
		if (self.curAI == 'AIDRONEWAIT')
		{
			if (self.actioncounter >= 4)
			{
				if (Duke.rnd(16))
				{
					if (self.cansee(p))
					{
						self.PlayActorSound("DRON_ATTACK1");
						if (self.checkp(p, phigher))
						{
							setAI('AIDRONEGETUP');
						}
						else
						{
							setAI('AIDRONEGETE');
						}
					}
				}
			}
		}
		else if (self.curAI == 'AIDRONEGETE')
		{
			state_dronegetstate(p, pdist);
		}
		else if (self.curAI == 'AIDRONEGETUP')
		{
			state_dronegetstate(p, pdist);
		}
		else if (self.curAI == 'AIDRONEPULLBACK')
		{
			if (self.counter >= 32)
			{
				setAI('AIDRONEWAIT');
			}
		}
		else if (self.curAI == 'AIDRONEHIT')
		{
			if (self.counter >= 8)
			{
				setAI('AIDRONEWAIT');
			}
		}
		else if (self.curAI == 'AIDRONESHRUNK')
		{
			state_droneshrunkstate(p, pdist);
		}
		else if (self.curAI == 'AIDRONEDODGE')
		{
			state_dronedodgestate(p, pdist);
		}
		else if (self.curAI == 'AIDRONEDODGEUP')
		{
			state_dronedodgestate(p, pdist);
		}
		if (self.ifhitbyweapon() >= 0)
		{
			state_checkdronehitstate(p, pdist);
		}
		if (Duke.rnd(1))
		{
			self.PlayActorSound("DRON_ROAM", CHAN_AUTO, CHANF_SINGULAR);
		}
	}
}
