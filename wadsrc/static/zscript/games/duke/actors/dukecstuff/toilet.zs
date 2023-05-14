
class DukeStall : DukeActor
{
	default
	{
		spriteset "STALL", "STALLBROKE";
		precacheclass "DukeToiletWater";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.OwnerActor = self;
		self.lotag = 1;
		self.clipdist = 2;
	}

	override bool OnUse(DukePlayer p)
	{
		if (self.spritesetindex == 0)
		{
			let pact = p.actor;
			if (p.last_pissed_time == 0)
			{
				pact.PlayActorSound("PLAYER_URINATE");
				p.last_pissed_time = 26 * 220;
				p.transporter_hold = 29 * 2;
				if (p.holster_weapon == 0)
				{
					p.holster_weapon = 1;
					p.weapon_pos = -1;
				}
				if (pact.extra <= (gs.max_player_health - (gs.max_player_health / 10)))
				{
					pact.extra += gs.max_player_health / 10;
					p.last_extra = pact.extra;
				}
				else if (pact.extra < gs.max_player_health)
					pact.extra = gs.max_player_health;
			}
			else if (self.CheckSoundPlaying("FLUSH_TOILET") == 0)
				self.PlayActorSound("FLUSH_TOILET");
			return true;
		}
		return false;
	}

	override void OnHit(DukeActor proj)
	{
		if (self.getSpritesetSize() > 1)
		{
			self.setSpriteSetImage(1);
			if (random(0, 1)) self.cstat |= CSTAT_SPRITE_XFLIP;
			self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			self.spawn("DukeToiletWater");
			self.PlayActorSound("GLASS_BREAKING");
		}
	}

}

class DukeToilet : DukeStall
{
	default
	{
		spriteset "TOILET", "TOILETBROKE";
	}

	override void StandingOn(DukePlayer p)
	{
		if (isRR() && p.PlayerInput(Duke.SB_CROUCH) && !p.OnMotorcycle)
		{
			p.actor.PlayActorSound("CRAP");
			p.last_pissed_time = 4000;
			p.eat = 0;
		}
	}
}

class RedneckToilet2 : DukeToilet
{
	default
	{
		spriteset "TOILET2";
	}
}

class RedneckToiletSeat : DukeToilet
{
	default
	{
		spriteset "TOILETSEAT";
	}
}

