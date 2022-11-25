
class RedneckBell : DukeActor
{
	default
	{
		pic "BIGBELL";
	}

	override void OnHit(DukeActor proj)
	{
		self.PlayActorSound("BELL");
	}
}