
// dummy items representing certain weapons
class DukeChaingun : DukeActor
{
	default
	{
		pic "CHAINGUN";
	}
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
