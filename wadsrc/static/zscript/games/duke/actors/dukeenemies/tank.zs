class DukeTank : DukeActor
{
	const TANKSTRENGTH = 500;
	meta class<DukeActor> spawntype;
	property spawntype: spawntype;

 	default
	{
		pic "TANK";
		Strength TANKSTRENGTH;
		+BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+NORADIUSPUSH;
		DukeTank.SpawnType "DukePigCop";
	}
}
