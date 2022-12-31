
class DukeCannonball: DukeActor
{
	const CANNONBALLSTRENGTH = 400;
	default
	{
		pic "CANNONBALL";
		Strength CANNONBALLSTRENGTH;
	}
	
}

class DukeCannonballs : DukeActor // (1818)
{
	const CANNONBALLSSTRENGTH = 10;
	default
	{
		pic "CANNONBALLS";
		Strength CANNONBALLSSTRENGTH;
	}
	
}

class DukeCannon : DukeActor // (1810)
{
	const CANNONSTRENGTH = 400;
	default
	{
		+BADGUY
		Strength CANNONSTRENGTH;
		pic "CANNON";
	}
}


