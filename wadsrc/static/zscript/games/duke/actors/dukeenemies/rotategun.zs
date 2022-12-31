
class DukeRotateGun : DukeActor
{
	const ROTTURRETSTRENGTH = 40;
	
	default
	{
		pic "ROTATEGUN";
		Strength ROTTURRETSTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+NORADIUSPUSH;
		+SHOOTCENTERED;
		+NOVERTICALMOVE;
		+MOVE_NOPLAYERINTERACT;
		+NOHITJIBS;
		
		sparkoffset -8;
		aimoffset 32;
		shootzoffset 0;
	}
	
	override void Initialize()
	{
		self.vel.Z = 0;
	}

}