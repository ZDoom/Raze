
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
		+NOSHOTGUNBLOOD;
		
		sparkoffset -3;
		aimoffset 32;
		shootzoffset 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.vel.Z = 0;
	}

}