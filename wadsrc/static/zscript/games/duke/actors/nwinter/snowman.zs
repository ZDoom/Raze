class NWinterSnowball : DukeActor
{
	default
	{
		pic "SNOWBALL";
		Strength 0;
	}
	
}



class NWinterSnowman : DukeActor
{
	const SNOWMANSTRENGTH = 75;
	
	default
	{
		pic "SNOWMAN";
		+BADGUY
		+KILLCOUNT
		Strength SNOWMANSTRENGTH;
	}
	
	
}

class NWinterTank : DukeTank
{
	default
	{
		DukeTank.SpawnType "NWinterSnowman";
	}
}
