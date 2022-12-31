//---------------------------------------------------------------------------
//
// this class is called shitball - but it's not just about throwing shit in the game,
// the entire logic with 4 different looks depending on the shooter is also shit...
//
//---------------------------------------------------------------------------

class RedneckShitBall : DukeSpit
{
	default
	{
		spriteset "SHITBALL", "SHITBALL2", "SHITBALL3", "SHITBALL4", 
			"FROGBALL1", "FROGBALL2", "FROGBALL3", "FROGBALL4", "FROGBALL5", "FROGBALL6", 
			"SHITBURN", "SHITBURN2", "SHITBURN3", "SHITBURN4",
			"RABBITBALL";
		+NOFLOORPAL;
		Strength SHITBALL_WEAPON_STRENGTH;
	}
	
	private void rabbitguts()
	{
		self.spawnguts('RedneckRabbitJibA', 2);
		self.spawnguts('RedneckRabbitJibB', 2);
		self.spawnguts('RedneckRabbitJibC', 2);

	}
	override bool weaponhitplayer(DukeActor targ)
	{
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();

		return Super.weaponhitplayer(targ);
	}
	
	override bool weaponhitwall(walltype wal)
	{
		self.SetPosition(oldpos);
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();
		
		return super.weaponhitwall(wal);
	}	
	
	override bool weaponhitsector()
	{
		self.setPosition(oldpos);
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();

		return super.weaponhitsector();
	}
		
	override bool animate(tspritetype tspr)
	{
		int sprite = ((PlayClock >> 4) & 3);
		if (self.ownerActor)
		{
			let OwnerAc = self.ownerActor;
			if (OwnerAc.bTRANSFERPALTOJIBS)
			{
				if (OwnerAc.pal == 8)
				{
					sprite = 4 + ((PlayClock >> 4) % 6);
				}
				else if (OwnerAc.pal == 19)
				{
					sprite = 10 + ((PlayClock >> 4) & 3);
					tspr.shade = -127;
				}
			}
			else if (OwnerAc.bSPAWNRABBITGUTS)
			{
				tspr.clipdist |= TSPR_ROTATE8FRAMES;
				sprite = 14;
			}
		}
		return true;
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		if (actor.bSPAWNRABBITGUTS)
		{
			shootprojectile1(actor, p, pos, ang, 37.5, -20);
		}
		else
		{
			shootprojectile1(actor, p, pos, ang, 25, -10);
		}
		return true;
	}
	
}
