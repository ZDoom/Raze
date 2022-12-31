class RedneckPlayerPawn : DukePlayerBase
{
	default
	{
		+DESTRUCTOIMMUNE;		
		Strength MAXPLAYERHEALTH;
	}
}

class RedneckPlayerLyingDead : DukeActor // LNRDLYINGDEAD (3998)
{
	default
	{
		Strength 0;
		action "PLYINGFRAMES", 0, 1, 0, 1, 1;
		StartAction "PLYINGFRAMES";
	}
	override bool animate(tspritetype t)
	{
		t.scale = (0.375, 0.265625);
		if (self.extra > 0)
			t.pos.Z += 6;
		return false;
	}
}
