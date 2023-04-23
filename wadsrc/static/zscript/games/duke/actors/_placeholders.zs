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


class DukeBurnedCorpse : DukeActor
{
	default
	{
		pic "BURNEDCORPSE";
		+FORCERUNCON;
	}
}

class DukeSpeaker : DukeActor
{
	default
	{
		pic "SPEAKER";
		+NOFALLER;
	}
}
class DukeFem6Pad: DukeActor
{
	default
	{
		pic "FEM6PAD";
	}
}



class DukeHotMeat : DukeActor // (4427)
{
	default
	{
		pic "HOTMEAT";
	}
}

class DukeLavaBubble : DukeActor // (4340)
{
	default
	{
		pic "LAVABUBBLE";
	}
}

