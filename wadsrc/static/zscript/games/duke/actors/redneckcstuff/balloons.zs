
class RedneckBalloons1 : DukeActor
{
	Default
	{
		Spriteset "BALLOONS1", "BALLOONS1BROKE";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.Scale = (0.5, 0.5);
		self.extra = 0;
		self.hitag = 0;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		if (self.hitag > 2)
			self.hitag = 0;
		
		if (spritesetindex == 1 && self.hitag != 2)
		{
			self.hitag = 2;
			self.extra = -100;
		}
		if (self.hitag == 0)
		{
			self.extra++;
			if (self.extra >= 30)
				self.hitag = 1;
		}
		else if (self.hitag == 1)
		{
			self.extra--;
			if (self.extra <= -30)
				self.hitag = 0;
		}
		else if (self.hitag == 2)
		{
			self.extra--;
			if (self.extra <= -104)
			{
				self.spawnsprite(self.lotag);
				self.Destroy();
			}
		}
		self.movesprite((0, 0, self.extra / 128.), CLIPMASK0);
	}
	
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0) 
		{
			self.SetSpritesetImage(1);
			self.playActorSound("BALLOON");
		}
	}
}

class RedneckBalloons2 : DukeActor
{
	Default
	{
		Spriteset "BALLOONS2", "BALLOONS2BROKE";
	}
}
