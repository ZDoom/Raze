
// The gator is the only new thing in Route 66.
class RedneckGator : DukeActor
{
	const GATOR_STRENGTH = 100;
	const GATOR_BITE = -5;
	const GATOR_GOOD_BITE = -10;

	default
	{
		pic "GATOR";
		Strength GATOR_STRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
}