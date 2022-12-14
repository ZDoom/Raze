class DukeDrone : DukeActor
{
	default
	{
		pic "DRONE";
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("DRON_RECOG");
	}
}

class DukeTurret : DukeActor
{
	default
	{
		pic "ORGANTIC";
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("TURR_RECOG");
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}
}

class DukeRotateGun : DukeActor
{
	default
	{
		pic "ROTATEGUN";
	}
	
	override void Initialize()
	{
			self.vel.Z = 0;
	}
}

class DukeShark : DukeActor
{
	default
	{
		pic "SHARK";
	}
	
	override void Initialize()
	{
		// override some defaults.
		self.scale = (0.9375, 0.9375);
		self.clipdist = 10;
	}
}


