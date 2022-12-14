class DukeSteamBase : DukeActor // we need this for in-game checking and the shared CON code.
{
	default
	{
		statnum STAT_STANDABLE;
	}
}

class DukeCeilingSteam : DukeSteamBase
{
	default
	{
		pic "CEILINGSTEAM";
	}
}


class DukeSteam : DukeActor
{
	default
	{
		pic "STEAM";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;

		if (owner && owner != self)
		{
			self.Angle = owner.Angle;
			self.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
			self.vel.X = -0.5;
			self.doMove(CLIPMASK0);
		}
		self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.ChangeStat(STAT_STANDABLE);
	}
}
