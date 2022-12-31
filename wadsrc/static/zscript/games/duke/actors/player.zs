class DukePlayerBase : DukeActor
{
	default
	{
		pic "APLAYER";
		+DESTRUCTOIMMUNE;
	}
}


class RedneckPlayerLyingDead : DukeActor
{
	override bool animate(tspritetype t)
	{
		t.scale = (0.375, 0.265625);
		if (self.extra > 0)
			t.pos.Z += 6;
		return false;
	}
}
