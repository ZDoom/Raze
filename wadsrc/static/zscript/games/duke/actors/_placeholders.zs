
// dummy items representing certain weapons

class DukeKneeAttack : DukeActor
{
	default
	{
		pic "KNEE";
		+DIENOW;
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
		+INFLAME;
		+DIENOW;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
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

class DukeShotSpark : DukeActor
{
	default
	{
		pic "SHOTSPARK1";
		+FORCERUNCON;
		+LIGHTDAMAGE;
		statnum STAT_MISC;
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
