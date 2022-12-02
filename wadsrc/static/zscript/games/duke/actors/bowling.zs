class RedneckBowlingPin : DukeActor
{
	default
	{
		statnum STAT_ZOMBIEACTOR;
		clipdist 12;
		scaleX 0.359375;
		scaleY 0.359375;
		detail 0;
		spriteset "BOWLINGPIN", "BOWLINGPIN1";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	
	protected void DoTick(int type)
	{
		if (self.sector.lotag == 900)
			self.vel.X = 0;
		
		if(self.vel.X != 0)
		{
			self.makeitfall();
			CollisionData coll;
			self.movesprite_ex((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0, coll);
			if (coll.type)
			{
				if (coll.type == kHitWall)
				{
					double k = coll.hitWall().delta().Angle();
					self.angle = k * 2 - self.angle;
				}
				else if (coll.type == kHitSprite)
				{
					let hitact = DukeActor(coll.hitActor());
					// avoid checkhitsprite here. The way this was handled was just wrong on all accounts
					self.collide(hitact);
					//if (hitact is "RedneckHen")	// does not work yet - Hen is not scriptified.
					if (hitact.checkType("HEN"))	// Temporary workaround
					{
						let ns = hitact.spawn("RedneckHenstand");
						hitact.scale = (0,0);
						hitact.ChangeStat(STAT_MISC);
						if (ns)
						{
							ns.vel.X = 2;
							ns.lotag = 40;
							ns.angle = self.angle;
						}
					}
				}
			}
			self.vel.X -= 1/16.;
			if(self.vel.X < 0) self.vel.X = 0;
			self.cstat = CSTAT_SPRITE_BLOCK_ALL;
			if (type < 2 && self.vel.X > 0)
			{
				self.cstat |= CSTAT_SPRITE_XFLIP & int(self.vel.X * 16);
				self.cstat |= CSTAT_SPRITE_YFLIP & int(self.vel.X * 16);
				if (random(0, 1)) self.setSpritesetImage(1);
			}
			if (type < 2 && self.vel.X == 0)
			{
				return;
			}
		}
		else if (self.sector.lotag == 900 && type != 2)
		{
			self.Destroy();
		}
	}
	
	override void Tick()
	{
		DoTick(0);
	}
	
	
	
	virtual void collide(DukeActor targa)
	{
		let targ = RedneckBowlingPin(targa);
		if (!targ) return;
		
		if (targ.detail == 0)
		{
			self.vel.X *= 0.75;
			self.angle -= targ.angle * 2 + frandom(0, 11.25);
			targ.angle += frandom(0, 22.5 / 8);
			targ.PlayActorSound("BOWLPIN");
		}
		else if (targ.detail == 1)
		{
			self.vel.X *= 0.75;
			self.angle -= targ.angle * 2 + frandom(0, 22.5 / 8);
			targ.angle += frandom(0, 22.5 / 8);
			targ.PlayActorSound("BOWLPIN");
		}
	}
	
	override void onHit(DukeActor hitter)
	{
		if (random(0, 3))
		{
			self.vel.X = 10.25;
			self.angle = hitter.angle;
		}
	}
}

// chickens on the bowling lane...
class RedneckHenstand : RedneckBowlingPin
{
	default
	{
		scaleY 0.234375;
		spriteset "HENSTAND", "HENSTAND1";
		detail 1;
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag == 0)
		{
			self.spawn("RedneckHen");
			self.scale = (0,0);
			self.ChangeStat(STAT_MISC);
			return;
		}
		DoTick(1);
	}
}

class RedneckBowlingBall : RedneckBowlingPin
{
	default
	{
		clipdist 17;
		scaleX 0.171875;
		scaleY 0.140625;
		statnum STAT_ACTOR;
		pic "BOWLINGBALL";
		detail 2;
	}
	
	override void Tick()
	{
		if (self.vel.X != 0)
		{
			if(!Duke.CheckSoundPlaying("BOWLLOOP"))
				self.PlayActorSound("BOWLLOOP");
		}
		else
		{
			self.spawn("RedneckBowlingBallSprite");
			self.Destroy();
			return;
		}
		if (self.sector.lotag == 900)
		{
			self.StopSound("BOWLLOOP");
		}
		DoTick(2);
		if (self.sector.lotag == 900)
		{
			self.ballreturn();
			self.Destroy();
			return;
		}
	}
	
	private void ballreturn()
	{
		DukeStatIterator it;
		for (let act1 = it.First(STAT_BOWLING); act1; act1 = it.Next())
		{
			if (act1.getClassName() == 'RedneckBowlingSectorLink' && self.sector == act1.sector)
			{
				DukeStatIterator it2;
				for (let act2 = it2.First(STAT_BOWLING); act2; act2 = it2.Next())
				{
					if (act2.getClassName() == 'RedneckBowlingBallSpot' && act1.hitag == act2.hitag)
					{
						act2.spawn("RedneckBowlingBallSprite");
					}
					if (act2.getClassName() == 'RedneckBowlingPinController' && act1.hitag == act2.hitag && act2.lotag == 0)
					{
						let sec = act2.sector;
						act2.lotag = 100;
						act2.extra++;
						int j = dlevel.getanimationindex(dlevel.anim_ceilingz, sec);
						if (j == -1)
							dlevel.setanimation(sec, dlevel.anim_ceilingz, sec, sec.floorz, 0.25);
					}
				}
			}
		}
	}
	
	override void onHit(DukeActor hitter)
	{
	}

	override void collide(DukeActor targ)
	{
		targ.vel.X = self.vel.X * 0.75;
		if (random(0, 32767) & 16) targ.angle -= 22.5 / 8;
		targ.PlayActorSound("BOWLPIN");
	}

	override bool ShootThis(DukeActor actor, DukePlayer plr, Vector3 spos, double sang)
	{
		let j = actor.spawn(self.GetClassName());
		if (j)
		{
			j.vel.X = 250 / 16.;
			j.angle = self.angle;
			j.pos.Z -= 15;
		}
		return true;
	}
}

class RedneckBowlingController : DukeActor
{
	default
	{
		statnum STAT_BOWLING;
		scaleX 0;
		scaleY 0;
		clipdist 0;
		extra 0;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
	}
}

class RedneckBowlingPinController : RedneckBowlingController
{
	override void Tick()
	{
		if (self.lotag == 100)
		{
			let pst = pinsectorresetup();
			if (pst)
			{
				self.lotag = 0;
				if (self.extra == 1)
				{
					pst = checkpins();
					if (!pst)
					{
						self.extra = 2;
					}
				}
				if (self.extra == 2)
				{
					self.extra = 0;
					resetpins();
				}
			}
		}
	}
	
	int pinsectorresetup()
	{
		let sec = self.sector;
		int j = dlevel.getanimationindex(dlevel.anim_ceilingz, sec);

		if (j == -1)
		{
			double z = sec.nextsectorneighborz(sec.ceilingz, sectortype.Find_CeilingUp | sectortype.Find_Safe).ceilingz;
			dlevel.setanimation(sec, dlevel.anim_ceilingz, sec, z, 0.25);
			return 1;
		}
		return 0;
	}

	int checkpins()
	{
		int pins = 0;
		int pin = 0;

		DukeSectIterator it;
		for (let a2 = it.First(self.sector); a2; a2 = it.Next())
		{
			if (a2.GetClassName() == 'RedneckBowlingPin' && a2.spritesetindex == 0)
			{
				pin++;
				pins |= 1 << a2.lotag;
			}
		}

		Duke.updatepindisplay(self.hitag, pins);
		return pin;
	}

	void resetpins()
	{
		int i, tag = 0;
		DukeSectIterator it;
		for (let a2 = it.First(self.sector); a2; a2 = it.Next())
		{
			if (a2.GetClassName() == 'RedneckBowlingPin')
				a2.Destroy();
		}
		for (let a2 = it.First(self.sector); a2; a2 = it.Next())
		{
			if (a2.GetClassName() == 'RedneckBowlingPinSpot')
			{
				let spawned = a2.spawn('RedneckBowlingPin');
				if (spawned)
				{
					spawned.lotag = a2.lotag;
					spawned.clipdist = 12;	// random formula here was bogus and always produced 48.
					spawned.angle -= 22.5 * 0.125 * (((random(0, 32767) & 32) - (random(0, 32767) & 64)) >> 5);
				}
			}
		}
		Duke.updatepindisplay(self.hitag, 0xffff);
	}
}

class RedneckBowlingSectorLink : RedneckBowlingController
{ }

class RedneckBowlingBallSpot : RedneckBowlingController
{ }

class RedneckBowlingPinSpot : RedneckBowlingController
{ }

