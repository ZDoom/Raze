

class DukeRespawnMarker : DukeActor
{
	default
	{
		spriteset "RESPAWNMARKERRED", "RESPAWNMARKERYELLOW", "RESPAWNMARKERGREEN";
		scalex 0.375;
		scaley 0.375;
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned && ownerActor != null) self.pos.Z = ownerActor.floorZ;
	}
	
	override void Tick()
	{
		self.temp_data[0]++;
		if (self.temp_data[0] > gs.respawnitemtime)
		{
			self.Destroy();
			return;
		}
		if (self.temp_data[0] >= (gs.respawnitemtime >> 1) && self.temp_data[0] < ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
			self.setSpritesetImage(1);
		else if (self.temp_data[0] > ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
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
		scalex 0.125;
		scaley 0.125;
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