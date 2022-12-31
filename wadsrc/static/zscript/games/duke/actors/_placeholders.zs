class DukeRadiusExplosion : DukeActor
{
	default
	{
		pic "RADIUSEXPLOSION";
		+INFLAME;
		+DIENOW;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
	}
}

class DukeSectorEffector : DukeActor
{
	//This never gets ticked, the handler goes directly to the native implementations.
}

class RedneckMotoHit : DukeActor
{
	default
	{
		pic "MOTOHIT";
	}
}

