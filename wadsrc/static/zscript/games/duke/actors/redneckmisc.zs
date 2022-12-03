
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


class RedneckMusicNotes : DukeActor
{
	default
	{
		pic "MUSICNOTES";
		lotag -1;
		clipdist 0;
	}
	
	override bool OnUse(DukePlayer user)
	{
		if (!self.CheckSoundPlaying("PIANO_P3"))
			self.PlayActorSound("PIANO_P3");
		return true;
	}
}

class RedneckJoe9000 : DukeActor
{
	default
	{
		pic "JOE9000";
		lotag 1;
		clipdist 0;
	}
	
	override bool OnUse(DukePlayer user)
	{
		if (ud.multimode < 2)
		{
			if (ud.joe9000 != 0)
			{
				if (!self.CheckSoundPlaying("JOE9000A") && !self.CheckSoundPlaying("JOE9000B") &&  !self.CheckSoundPlaying("JOE9000C"))
				{
					if (random(0, 1))
						self.PlayActorSound("JOE9000B");
					else
						self.PlayActorSound("JOE9000C");
				}
			}
			else
			{
				self.PlayActorSound("JOE9000A");
				ud.joe9000 = 1;
			}
		}
		return true;
	}
}
