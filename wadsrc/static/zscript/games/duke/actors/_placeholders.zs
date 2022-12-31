class DukeSectorEffector : DukeActor
{
	//This never gets ticked, the handler goes directly to the native implementations.
}

class DukePlayerBase : DukeActor
{
	const YELLHURTSOUNDSTRENGTH = 40;
	const YELLHURTSOUNDSTRENGTHMP = 50;
	default
	{
		pic "APLAYER";
		+DESTRUCTOIMMUNE;
	}
}

class RedneckMotoHit : DukeActor
{
	default
	{
		pic "MOTOHIT";
	}
}

