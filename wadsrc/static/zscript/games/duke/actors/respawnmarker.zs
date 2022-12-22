

class DukeRespawnMarker : DukeActor
{
	default
	{
		spriteset "RESPAWNMARKERRED", "RESPAWNMARKERYELLOW", "RESPAWNMARKERGREEN";
		+NOFLOORPAL;
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned && ownerActor != null) self.pos.Z = ownerActor.floorZ;
		self.scale = (0.375, 0.375);
	}
	
	override void Tick()
	{
		self.counter++;
		if (self.counter > gs.respawnitemtime)
		{
			self.Destroy();
			return;
		}
		if (self.counter >= (gs.respawnitemtime >> 1) && self.counter < ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
			self.setSpritesetImage(1);
		else if (self.counter > ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
			self.setSpritesetImage(2);
		self.makeitfall();
	}
	
	override bool Animate(tspritetype t)
	{
		if (ud.marker == 0)
			t.scale = (0, 0);
		return true;
	}
}

class RedneckRespawnMarker : DukeRespawnMarker
{
	default
	{
		spriteset "RESPAWNMARKER1", "RESPAWNMARKER2", "RESPAWNMARKER3", "RESPAWNMARKER4", "RESPAWNMARKER5", "RESPAWNMARKERRED", "RESPAWNMARKER7", 
			"RESPAWNMARKER8", "RESPAWNMARKER9", "RESPAWNMARKER10", "RESPAWNMARKER11", "RESPAWNMARKER12", "RESPAWNMARKER13", "RESPAWNMARKER14";
	}

	override void Initialize()
	{
		self.scale = (0.125, 0.125);
	}

	override bool Animate(tspritetype t)
	{
		t.setSpritePic(self, ((PlayClock >> 4) % 14));
		t.pal = self.spriteSetIndex;
		if (ud.marker == 0)
			t.scale = (0, 0);
		
		return true;
	}
	
}