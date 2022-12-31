class NWinterElfUzi : DukeActor
{
	const ELFUZISTRENGTH = 50;
	const ELFUZIALTSTR = 25;
	
	default
	{
		pic "ELFUZI";
		+BADGUY
		+KILLCOUNT
		Strength ELFUZISTRENGTH;

	}
	
	
}

class NWinterElfUziStayput : DukeActor
{
	default
	{
		pic "ELFUZISTAYPUT";
		+BADGUYSTAYPUT;
	}
	
}

class NWinterSpecBlood : DukeActor
{
	default
	{
		pic "SPECBLOOD";
	}
	
	
}
