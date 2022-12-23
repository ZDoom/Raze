
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
	}
	
	override void Initialize()
	{
		self.lotag = -1;
		self.clipdist = 0;
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
	}
	
	override void Initialize()
	{
		self.lotag = 1;
		self.clipdist = 0;
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

class RedneckBellSwitch : DukeActor
{
	default
	{
		spriteset "BELLSWITCH", "BELLSWITCHON", "BELLSWITCHOFF";
	}
	
	override bool TriggerSwitch(DukePlayer activator)
	{
		if (self.spritesetindex == 1 || dlevel.check_activator_motion(lotag)) return true;
	
		self.detail = 132;
		self.setSpriteSetImage(1);
		self.changeStat(STAT_MISC);	// needs to be made to call Tick
		return 2; // 2 lets the switch code perform the trigger action without messing around with this actor.
	}
	
	override void Tick()
	{
		if (self.detail > 0)
		{
			self.detail--;
			if (self.detail == 0)
				self.SetSpritesetImage(2);	// stop animating
		}
	}
}	

class RedneckLetsBowl : DukeActor
{
	default
	{
		spriteset "LETSBOWL", "LETSBOWL1";
	}

	override bool animate(tspritetype t)
	{
		t.SetspritePic(self, (PlayClock >> 2) & 1);
		return true;
	}
}

class RedneckWacoWindow : DukeActor
{
	default
	{
		pic "WACOWINDER";
		+BADGUY;
	}
}


class RedneckTeslaBall : DukeItemBase
{
	default
	{
		pic "TESLABALL";
	}
}

class RedneckBuzzSaw : DukeItemBase
{
	default
	{
		pic "BUZSAW";
	}
}

class RedneckBustaWin5a : DukeItemBase
{
	default
	{
		pic "BUSTAWIN5A";
	}
}

class RedneckBustaWin4a : DukeItemBase
{
	default
	{
		pic "BUSTAWIN4A";
	}
}

class RedneckTesla : DukeItemBase
{
	default
	{
		pic "TESLA";
		+FULLBRIGHT
	}
}

class RedneckTikiLamp : DukeItemBase
{
	default
	{
		pic "TIKILAMP";
		+FULLBRIGHT
	}
}
