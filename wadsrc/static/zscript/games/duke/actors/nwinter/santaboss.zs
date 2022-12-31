class NWinterSanta : DukeActor
{
	const SOBBOTSTRENGTH = 2500;
	const MINIBOSSSTRENGTH = 100;

	default
	{
		+BADGUY;
		+KILLCOUNT;
		pic "PIGCOP";	// tiles are offset from here.
	}
	
}


class NWinterSantaFly : NWinterSanta
{
	default
	{
		pic "PIGCOPDIVE";	// tiles are offset from here.
		Strength SOBBOTSTRENGTH;
		StartAction "ABOTFLY";
	}
	
}
