class DukeRobotmouse : DukeActor // ROBOTMOUSE (4407)
{
	const ROBOTMOUSESTRENGTH = 45;

	default
	{
		pic "ROBOTMOUSE";
		Strength ROBOTMOUSESTRENGTH;
		move "MOUSEVELS", 32;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.temp_data[0] == 0)
		{
			self.temp_data[0] = 1;
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			self.clipdist += 64 * 0.25;
		}
		else
		{
			if (self.extra < 0)
			{
				if (self.counter >= 32)
				{
					p.actor.PlayActorSound("MOUSEANNOY", CHAN_AUTO, CHANF_LOCAL);
					self.killit();
				}
				return;
			}
			if (self.counter >= 64)
			{
				if (Duke.rnd(6))
				{
					if (Duke.rnd(128))
					{
						setMove('MOUSEVELS', randomangle | geth);
					}
					else
					{
						if (Duke.rnd(64))
						{
							self.PlayActorSound("HAPPYMOUSESND1", CHAN_AUTO, CHANF_SINGULAR);
						}
						else if (Duke.rnd(64))
						{
							self.PlayActorSound("HAPPYMOUSESND2", CHAN_AUTO, CHANF_SINGULAR);
						}
						else if (Duke.rnd(64))
						{
							self.PlayActorSound("HAPPYMOUSESND3", CHAN_AUTO, CHANF_SINGULAR);
						}
						else
						{
							self.PlayActorSound("HAPPYMOUSESND4", CHAN_AUTO, CHANF_SINGULAR);
						}
					}
					self.counter = 0;
				}
			}
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				self.spawndebris(DukeScrap.Scrap2, 10);
				self.tempval = self.pal;
				self.pal = 1;
				self.spawndebris(DukeScrap.Scrap3, 4);
				self.counter = 0;
				self.cstat = CSTAT_SPRITE_INVISIBLE;
			}
		}
		else
		{
			self.xoffset = self.yoffset = 0;
			self.fall(p);
		}
	}
}


// these virtuals need to be on DukeActor to ensure that ChangeType is safe for breakables.
extend class DukeActor
{
	virtual void BrkKilled(DukePlayer p, double pdist) {}
	virtual void BrkHit() {}
}

class DukeScriptedBreakable : DukeActor
{
	const PIRATEGALSTRENGTH = 200;
	const DOLPHINSTRENGTH = 50;

	void state_jibfood()
	{
		self.PlayActorSound("SQUISHED");
		self.spawnguts('DukeJibs6', 3);
		self.killit();
	}

	void BrkSound()
	{
		if (Duke.rnd(128))
		{
			self.PlayActorSound("GLASS_BREAKING");
		}
		else
		{
			self.PlayActorSound("GLASS_HEAVYBREAK");
		}
	}

	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spriteglass(10);
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.temp_data[0] == 0)
		{
			self.temp_data[0] = 1;
			self.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
		}

		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0)
			{
				BrkKilled(p, pdist);
				if (self.extra < 0) self.killit(); // not if it revived itself.
			}
			else BrkHit();
		}
		else if (!self.bNoGravity)
		{
			self.fall(p);
		}
	}
	
}

class DukeBurger : DukeScriptedBreakable // DUKEBURGER (4570)
{
	default
	{
		pic "DUKEBURGER";
		Strength PIRATEGALSTRENGTH;
//		action "ABURGERROTS", 0, 1, 5;
//		StartAction "ABURGERROTS";
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawndebris(DukeScrap.Scrap3, 14);
		self.spawndebris(DukeScrap.Scrap1, 13);
		self.spawndebris(DukeScrap.Scrap4, 12);
		self.spawndebris(DukeScrap.Scrap2, 12);
		self.spawndebris(DukeScrap.Scrap5, 11);
	}
}

class DukeMop : DukeScriptedBreakable // MOP (4497)
{
	default
	{
		pic "MOP";
		Strength WEAK;
	}
}

class DukeBroom : DukeScriptedBreakable // BROOM (4496)
{
	default
	{
		pic "BROOM";
		Strength WEAK;
	}
}
class DukeWetFloor : DukeScriptedBreakable // WETFLOOR (4495)
{
	default
	{
		pic "WETFLOOR";
		Strength WEAK;
	}
}
class DukeDeskLamp : DukeScriptedBreakable // DESKLAMP (4370)
{
	default
	{
		pic "DESKLAMP";
		Strength WEAK;
	}
}
class DukeHatRack : DukeScriptedBreakable // HATRACK (4367)
{
	default
	{
		pic "HATRACK";
		Strength WEAK;
	}
}
class DukeCoffeeMachine : DukeScriptedBreakable // COFFEEMACHINE (4372)
{
	default
	{
		pic "COFFEEMACHINE";
		Strength WEAK;
	}
}
class DukeGunpowderbarrel : DukeScriptedBreakable // GUNPOWDERBARREL (4360)
{
	default
	{
		pic "GUNPOWDERBARREL";
		Strength TOUGH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		if (self.sector != null) self.spawn('DukeExplosion2');
		self.PlayActorSound("PIPEBOMB_EXPLODE");
		self.hitradius(2048, WEAKEST, WEAK, MEDIUMSTRENGTH, TOUGH);
		if (pdist < 2048 * maptoworld)
		{
			p.wackplayer();
		}
		if (pdist > MAXSLEEPDISTF && self.timetosleep == 0) self.timetosleep = SLEEPTIME;
		self.spawndebris(DukeScrap.Scrap1, 10);
		self.spawndebris(DukeScrap.Scrap2, 13);
		self.spawndebris(DukeScrap.Scrap3, 4);
		self.spawndebris(DukeScrap.Scrap4, 17);
		self.spawndebris(DukeScrap.Scrap5, 6);
	}
}
class DukeFoodObject1 : DukeScriptedBreakable // FOODOBJECT1 (4530)
{
	default
	{
		pic "FOODOBJECT1";
		Strength WEAK;
	}
}
class DukeFoodObject2 : DukeScriptedBreakable // FOODOBJECT2 (4531)
{
	default
	{
		pic "FOODOBJECT2";
		Strength WEAK;
		+NOGRAVITY;
	}
}
class DukeFoodObject3 : DukeScriptedBreakable // FOODOBJECT3 (4532)
{
	default
	{
		pic "FOODOBJECT3";
		Strength WEAK;
	}
}
class DukeFoodObject4 : DukeScriptedBreakable // FOODOBJECT4 (4533)
{
	default
	{
		pic "FOODOBJECT4";
		Strength WEAK;
	}
}
class DukeFoodObject5 : DukeScriptedBreakable // FOODOBJECT5 (4534)
{
	default
	{
		pic "FOODOBJECT5";
		Strength WEAK;
	}
}
class DukeFoodObject6 : DukeScriptedBreakable // FOODOBJECT11 (4540)
{
	default
	{
		pic "FOODOBJECT6";
		Strength WEAK;
		+NOGRAVITY;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		state_jibfood();
	}
}
class DukeFoodObject7 : DukeScriptedBreakable // FOODOBJECT7 (4536)
{
	default
	{
		pic "FOODOBJECT7";
		Strength WEAK;
	}
}
class DukeFoodObject8 : DukeScriptedBreakable // FOODOBJECT8 (4537)
{
	default
	{
		pic "FOODOBJECT8";
		Strength WEAK;
	}
}
class DukeFoodObject9 : DukeScriptedBreakable // FOODOBJECT9 (4538)
{
	default
	{
		pic "FOODOBJECT9";
		Strength WEAK;
	}
}
class DukeFoodObject10 : DukeScriptedBreakable // FOODOBJECT10 (4539)
{
	default
	{
		pic "FOODOBJECT10";
		Strength WEAK;
	}
}
class DukeFoodObject11 : DukeScriptedBreakable // FOODOBJECT11 (4540)
{
	default
	{
		pic "FOODOBJECT11";
		Strength WEAK;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		state_jibfood();
	}
}
class DukeFoodObject12 : DukeFoodObject11 // FOODOBJECT12 (4541)
{
	default
	{
		pic "FOODOBJECT12";
		Strength WEAK;
	}
}
class DukeFoodObject13 : DukeFoodObject11 // FOODOBJECT13 (4542)
{
	default
	{
		pic "FOODOBJECT13";
		Strength WEAK;
	}
}
class DukeFoodObject14 : DukeFoodObject11 // FOODOBJECT14 (4543)
{
	default
	{
		pic "FOODOBJECT14";
		Strength WEAK;
	}
}
class DukeFoodObject15 : DukeFoodObject11 // FOODOBJECT15 (4544)
{
	default
	{
		pic "FOODOBJECT15";
		Strength WEAK;
	}
}
class DukeFoodObject16 : DukeFoodObject11 // FOODOBJECT16 (4545)
{
	default
	{
		pic "FOODOBJECT16";
		Strength WEAK;
	}
}
class DukeFoodObject17 : DukeFoodObject11 // FOODOBJECT17 (4546)
{
	default
	{
		pic "FOODOBJECT17";
		Strength WEAK;
	}
}
class DukeFoodObject18 : DukeScriptedBreakable // FOODOBJECT18 (4547)
{
	default
	{
		pic "FOODOBJECT18";
		Strength WEAK;
	}
}
class DukeFoodObject19 : DukeScriptedBreakable // FOODOBJECT19 (4548)
{
	default
	{
		pic "FOODOBJECT19";
		Strength WEAK;
	}
}
class DukeFoodObject20 : DukeScriptedBreakable // FOODOBJECT20 (4549)
{
	default
	{
		pic "FOODOBJECT20";
		Strength WEAK;
	}
}
class DukeSkinnedChicken : DukeFoodObject6 // SKINNEDCHICKEN (4554)
{
	default
	{
		pic "SKINNEDCHICKEN";
		Strength WEAK;
	}
}
class DukeFeatheredChicken : DukeFoodObject6 // FEATHEREDCHICKEN (4555)
{
	default
	{
		pic "FEATHEREDCHICKEN";
		Strength WEAK;
	}
}
class DukeTopSecret : DukeScriptedBreakable // TOPSECRET (4396)
{
	default
	{
		pic "TOPSECRET";
		Strength WEAK;
		+NOGRAVITY;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.lotsofstuff('DukePaper', 10);
	}
}

class DukeDolphin1 : DukeScriptedBreakable // DOLPHIN1 (4591)
{
	default
	{
		pic "DOLPHIN1";
		Strength DOLPHINSTRENGTH;
		+NOGRAVITY;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawnguts('DukeJibs2', 1);
		self.spawnguts('DukeJibs3', 2);
		self.spawnguts('DukeJibs4', 3);
		self.spawnguts('DukeJibs5', 2);
		state_jibfood();
	}
	
	override void BrkHit()
	{
		self.spawnguts('DukeJibs6', 1);
		self.PlayActorSound("DOLPHINSND", CHAN_AUTO, CHANF_SINGULAR);
		if (self.extra <= TOUGH) // ifstrength
		{
			self.ChangeType('DukeDolphin2');
			self.PlayActorSound("SQUISHED");
		}
	}
}
class DukeDolphin2 : DukeDolphin1 // DOLPHIN2 (4592)
{
	default
	{
		pic "DOLPHIN2";
	}

	override void BrkHit()
	{
		self.spawnguts('DukeJibs6', 1);
		self.PlayActorSound("DOLPHINSND", CHAN_AUTO, CHANF_SINGULAR);
	}
}

class DukeRobotDog2 : DukeScriptedBreakable // ROBOTDOG2 (4560)
{
	default
	{
		pic "ROBOTDOG2";
		Strength TOUGH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("DEAD_DOG", CHAN_AUTO, CHANF_SINGULAR);
		self.spawnguts('DukeJibs2', 1);
		self.spawnguts('DukeJibs3', 2);
		self.spawnguts('DukeJibs6', 3);
	}
	override void BrkHit()
	{
		self.spawnguts('DukeJibs6', 1);
		self.PlayActorSound("WHINING_DOG", CHAN_AUTO, CHANF_SINGULAR);
	}
	
}

class DukeClock : DukeScriptedBreakable // CLOCK (1060)
{
	default
	{
		pic "CLOCK";
		Strength WEAK;
		+NOGRAVITY;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.ChangeType('DukeBrokenClock');
		self.extra = 1;
	}
	
}

class DukeBrokenClock: DukeActor
{
	default
	{
		pic "BROKENCLOCK";
	}
}

class DukeTeddybear : DukeScriptedBreakable // TEDDYBEAR (4400)
{
	default
	{
		pic "TEDDYBEAR";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.tempval = self.pal;
		self.pal = 1;
		self.spawndebris(DukeScrap.Scrap3, 6);
	}
}
class DukePirate1A : DukeScriptedBreakable // PIRATE1A (4510)
{
	default
	{
		pic "PIRATE1A";
		Strength PIRATEGALSTRENGTH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawndebris(DukeScrap.Scrap2, 10);
		self.spawndebris(DukeScrap.Scrap1, 5);
		self.spawndebris(DukeScrap.Scrap3, 3);
		self.spriteglass(10);
	}
}
class DukePirate2A : DukePirate1A // PIRATE2A (4512)
{
	default
	{
		pic "PIRATE2A";
	}
}
class DukePirate3A : DukePirate1A // PIRATE3A (4514)
{
	default
	{
		pic "PIRATE3A";
	}
}
class DukePirate4A : DukePirate1A // PIRATE4A (4511)
{
	default
	{
		pic "PIRATE4A";
	}
}
class DukePirate5A : DukePirate1A // PIRATE5A (4513)
{
	default
	{
		pic "PIRATE5A";
	}
}
class DukePirate6A : DukePirate1A // PIRATE6A (4515)
{
	default
	{
		pic "PIRATE6A";
	}
}
class DukeMan : DukePirate1A // MAN (4871)
{
	default
	{
		pic "MAN";
		Strength PIRATEGALSTRENGTH;
	}
}
class DukeMan2 : DukePirate1A // MAN2 (4872)
{
	default
	{
		pic "MAN2";
		Strength PIRATEGALSTRENGTH;
	}
}
class DukeWoman : DukeScriptedBreakable // WOMAN (4874)
{
	default
	{
		pic "WOMAN";
		Strength PIRATEGALSTRENGTH;
	}
}
class DukeRobotpirate : DukeScriptedBreakable // ROBOTPIRATE (4404)
{
	default
	{
		pic "ROBOTPIRATE";
		Strength PIRATEGALSTRENGTH;
	}

	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawndebris(DukeScrap.Scrap2, 10);
		self.spawndebris(DukeScrap.Scrap1, 5);
		self.spawndebris(DukeScrap.Scrap3, 3);
		self.spriteglass(10);
	}
}
class DukePirateHalf : DukeScriptedBreakable // PIRATEHALF (4516)
{
	default
	{
		pic "PIRATEHALF";
		Strength TOUGH;
	}
}
class DukeChestOfGold : DukeScriptedBreakable // CHESTOFGOLD (4520)
{
	default
	{
		pic "CHESTOFGOLD";
		Strength WEAK;
	}
}
class DukeRobotDog : DukeScriptedBreakable // ROBOTDOG (4402)
{
	default
	{
		pic "ROBOTDOG";
		Strength PIRATEGALSTRENGTH;
	}
}
class DukePleaseWait : DukeScriptedBreakable // PLEASEWAIT (4887)
{
	default
	{
		pic "PLEASEWAIT";
	}
}
class DukeJollyMeal : DukeScriptedBreakable // JOLLYMEAL (4569)
{
	default
	{
		pic "JOLLYMEAL";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawn('DukeAtomicHealth');
		self.spawndebris(DukeScrap.Scrap3, 1);
		self.spawndebris(DukeScrap.Scrap4, 2);
	}
}
class DukeGumballMachine : DukeScriptedBreakable // GUMBALLMACHINE (4458)
{
	default
	{
		pic "GUMBALLMACHINE";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.ChangeType('DukeGumballMachineBroke');
		self.extra = 1;
		self.spawndebris(DukeScrap.Scrap4, 2);
		self.spriteglass(10);
	}
}
class DukeGumballMachineBroke : DukeScriptedBreakable // GUMBALLMACHINEBROKE (4459)
{
	default
	{
		pic "GUMBALLMACHINEBROKE";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawndebris(DukeScrap.Scrap3, 3);
		self.spawndebris(DukeScrap.Scrap4, 2);
		self.spriteglass(10);
	}	
}
class DukePoliceLightPole : DukeScriptedBreakable // POLICELIGHTPOLE (4377)
{
	default
	{
		pic "POLICELIGHTPOLE";
		Strength TOUGH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		self.spawndebris(DukeScrap.Scrap3, 4);
		self.spawndebris(DukeScrap.Scrap1, 3);
		self.spawndebris(DukeScrap.Scrap4, 2);
		self.spawndebris(DukeScrap.Scrap2, 2);
		self.spawndebris(DukeScrap.Scrap5, 1);
	}
}
class DukeMailbag : DukeScriptedBreakable // MAILBAG (4413)
{
	default
	{
		pic "MAILBAG";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.lotsofstuff('DukeMail', 30);
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
}
class DukeHeadLamp : DukeScriptedBreakable // HEADLAMP (4550)
{
	default
	{
		pic "HEADLAMP";
		Strength WEAK;
	}
}
class DukeSnakep : DukeScriptedBreakable // SNAKEP (4590)
{
	default
	{
		pic "SNAKEP";
		Strength MEDIUMSTRENGTH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawnguts('DukeJibs2', 1);
		self.spawnguts('DukeJibs3', 2);
		self.spawnguts('DukeJibs4', 3);
		self.spawnguts('DukeJibs5', 2);
		state_jibfood();
	}
}
class DukeDonuts : DukeScriptedBreakable // DONUTS (1045)
{
	default
	{
		pic "DONUTS";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.tempval = self.pal;
		self.pal = 7;
		self.spawnguts('DukeJibs6', 2);
	}
}
class DukeGavals : DukeScriptedBreakable // GAVALS (4374)
{
	default
	{
		pic "GAVALS";
		Strength WEAK;
	}
}
class DukeGavals2 : DukeScriptedBreakable // GAVALS2 (4375)
{
	default
	{
		pic "GAVALS2";
		Strength WEAK;
	}
}
class DukeCups : DukeScriptedBreakable // CUPS (4373)
{
	default
	{
		pic "CUPS";
		Strength WEAK;
	}
}
class DukeDonuts2 : DukeScriptedBreakable // DONUTS2 (4440)
{
	default
	{
		pic "DONUTS2";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawndebris(DukeScrap.Scrap1, 1);
		self.tempval = self.pal;
		self.pal = 7;
		self.spawnguts('DukeJibs6', 2);
	}
}
class DukeFloorbasket : DukeScriptedBreakable // FLOORBASKET (4388)
{
	default
	{
		pic "FLOORBASKET";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		BrkSound();
		if (self.sector != null) self.spawn('DukePuke');
		self.spawndebris(DukeScrap.Scrap1, 2);
		self.spawndebris(DukeScrap.Scrap3, 3);
		self.spawndebris(DukeScrap.Scrap4, 2);
	}
	
}
class DukeMeter : DukeScriptedBreakable // METER (4453)
{
	default
	{
		pic "METER";
		Strength WEAK;
	}
}
class DukeDeskPhone : DukeScriptedBreakable // DESKPHONE (4454)
{
	default
	{
		pic "DESKPHONE";
		Strength WEAK;
	}
}
class DukeMace : DukeScriptedBreakable // MACE (4464)
{
	default
	{
		pic "MACE";
		Strength WEAK;
	}
}
class DukeShoppingCart : DukeScriptedBreakable // SHOPPINGCART (4576)
{
	default
	{
		pic "SHOPPINGCART";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawndebris(DukeScrap.Scrap1, 5);
		self.spawndebris(DukeScrap.Scrap2, 5);
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.PlayActorSound("GLASS_HEAVYBREAK");
	}
}
class DukeCoffeeMug : DukeScriptedBreakable // COFFEEMUG (4438)
{
	default
	{
		pic "COFFEEMUG";
		Strength WEAK;
	}
}





