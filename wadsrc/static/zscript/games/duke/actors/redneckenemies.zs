class RedneckRock : DukeActor
{
	default
	{
		pic "ROCK";
		+INTERNAL_BADGUY;
	}
	override void Initialize()
	{
		self.scale = (1, 1);
		self.setClipDistFromTile();
	}
}

class RedneckRock2 : RedneckRock
{
	default
	{
		pic "ROCK2";
		+INTERNAL_BADGUY;
	}
}

class RedneckBoulder : DukeActor
{
	default
	{
		pic "BOULDER";
		+INTERNAL_BADGUY;
	}
}

class RedneckBoulder1 : DukeActor
{
	default
	{
		pic "BOULDER1";
		+INTERNAL_BADGUY;
	}
}
