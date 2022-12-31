
class DukeLavaBubble : DukeActor // (4340)
{
	default
	{
		pic "LAVABUBBLE";
	}
}

class DukeBurnedCorpse : DukeActor
{
	default
	{
		pic "BURNEDCORPSE";
		+FORCERUNCON;
	}
}

class DukeWhispySmoke : DukeActor
{
	default
	{
		pic "WHISPYSMOKE";
		+FORCERUNCON;
	}
	
	override void Initialize()
	{
		self.pos.X += frandom(-8, 8);
		self.pos.Y += frandom(-8, 8);
		self.scale = (0.3125, 0.3125);
		self.ChangeStat(STAT_MISC);
	}
}		
class DukeLavaSplash : DukeActor
{
	default
	{
		pic "LAVASPLASH";
	}
}		


