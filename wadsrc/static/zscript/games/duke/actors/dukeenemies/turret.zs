class DukeTurret : DukeActor
{
	const TURRETSTRENGTH = 30;

	default
	{
		pic "ORGANTIC";
		Strength TURRETSTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOVERTICALMOVE;
		+NOHITJIBS;
		aimoffset 32;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("TURR_RECOG");
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}

}