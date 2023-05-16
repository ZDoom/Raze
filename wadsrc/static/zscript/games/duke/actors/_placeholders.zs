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

	override void Initialize(DukeActor spawner)
	{
		// do not call the base class here.
	}
}

class RedneckMotoHit : DukeActor
{
	default
	{
		pic "MOTOHIT0";
	}
}

