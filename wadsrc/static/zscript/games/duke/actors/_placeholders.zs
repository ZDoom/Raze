
// dummy items representing certain weapons

class DukeKneeAttack : DukeActor
{
	default
	{
		pic "KNEE";
	}
}

class RedneckCrowbarAttack : DukeKneeAttack
{
}


class DukeRadiusExplosion : DukeActor
{
	default
	{
		pic "RADIUSEXPLOSION";
	}
}

class DukeShotgunShot : DukeActor
{
	default
	{
		pic "SHOTGUN";
	}
}

class RedneckShotgunShot : DukeShotgunShot
{
}


class DukeChaingunShot : DukeActor
{
	default
	{
		pic "CHAINGUN";
	}
}

class RedneckChaingunShot : DukeChaingunShot
{
}

class DukeSectorEffector : DukeActor
{
	//This never gets ticked, the handler goes directly to the native implementations.
}



// placeholders for CON scripted actors where we need the class.

class DukeForceRipple : DukeActor
{
	default
	{
		pic "FORCERIPPLE";
	}
}
