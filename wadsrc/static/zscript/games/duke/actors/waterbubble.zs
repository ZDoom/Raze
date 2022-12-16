class DukeWaterBubble : DukeActor
{
	default
	{
		pic "WATERBUBBLE";
		+FORCERUNCON;
		+NOFLOORPAL;
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner.isPlayer())
			self.pos.Z -= 16;
		if (owner != self) 
			self.angle = owner.angle;

		self.scale = (0.0625, 0.0625);
		self.ChangeStat(STAT_MISC);
	}

	override bool animate(tspritetype t)
	{
		if (dlevel.floorsurface(t.sector) == Duke.TSURF_SLIME)
		{
			t.pal = 7;
		}
		else
		{
			t.copyfloorpal(t.sector);
		}
		return false;
	}
}


