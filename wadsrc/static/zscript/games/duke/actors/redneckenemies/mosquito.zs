class RedneckMosquito : DukeActor
{
	const MOSQUITOSTRENGTH = 1;
	const MOSQDAMAGE = -4;
	default
	{
		pic "MOSQUITO";
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
	}

	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.21875, 0.109375);
		self.clipdist = 32;
	}

}
