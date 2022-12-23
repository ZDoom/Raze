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


// placeholders for CON scripted actors where we need the class.

class DukeForceRipple : DukeActor
{
	default
	{
		pic "FORCERIPPLE";
		+FORCERUNCON;
	}
}
