class DukeBoss5 : DukeBossBase
{
	const BOSS5STRENGTH = 6000;
	const BOSS5PALSTRENGTH = 1000;

	default
	{
		pic "BOSS5";
		-ALTHITSCANDIRECTION;
		Strength BOSS5STRENGTH;
	}
	
}


class DukeBoss5Stayput : DukeBoss5
{
	default
	{
		pic "BOSS5STAYPUT";
		+BADGUYSTAYPUT;
	}
}
	
