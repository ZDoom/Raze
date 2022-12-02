class RedneckPiano : DukeActor
{
	default
	{
		lotag 5;
		clipdist 0;
		statnum STAT_ACTOR;
		spriteset "PIANO", "PIANOBUST";
	}

	override void Tick()
	{
		if (self.lotag == 5)
			if (!Duke.CheckSoundPlaying("PIANO_P2"))
				self.PlayActorSound("PIANO_P2");
	}

	override void onHit(DukeActor hitter)
	{
		if (self.lotag == 5)
		{
			self.lotag = 0;
			self.setSpriteSetImage(1);
			self.PlayActorSound("PIANO_P3");
			DukeSpriteIterator it;
			for (let act = it.First(); act; act = it.Next())
			{
				if (act is 'RedneckPianoKeys') act.setSpriteSetImage(1);
			}
		}
	}
}

class RedneckPianoKeys : DukeActor
{
	default
	{
		spriteset "PIANOKEYS", "PIANOKEYSBUST";
	}
	
	override bool Animate(tspritetype tspr)
	{
		tspr.shade = self.shade;
		return true;
	}
}
