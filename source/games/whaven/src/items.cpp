	#include "ns.h"
#include "wh.h"
#include "gamestate.h"

BEGIN_WH_NS

Item items[MAXITEMS - ITEMSBASE];
	
boolean isItemSprite(DWHActor* actor) {
	return (actor->s().detail & 0xFF) >= ITEMSBASE && (actor->s().detail & 0xFF) < MAXITEMS;
}
	
void InitItems()
{
	int a = 0;
	items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // SILVERBAG
		{
			showmessage("Silver!", 360);
			DeleteActor(actor);
			SND_Sound(S_POTION1);
			addscore(&plr, krand() % 100 + 10);
		});

	items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // GOLDBAG
		{
			showmessage("Gold!", 360);
			DeleteActor(actor);
			SND_Sound(S_POTION1);
			addscore(&plr, krand() % 100 + 10);
		});

	items[a++].Init(27, 28, true, false, [](PLAYER& plr, DWHActor* actor) // HELMET
		{
			showmessage("Hero Time", 360);
			DeleteActor(actor);
			if (!isWh2())
				addarmor(plr, 10);
			plr.helmettime = 7200;
			// JSA_DEMO3
			treasuresfound++;
			SND_Sound(S_STING1 + krand() % 2);
			addscore(&plr, 10);
		});

	items[a++].Init(26, 26, true, false, [](PLAYER& plr, DWHActor* actor) // PLATEARMOR
		{
			if (plr.armor <= 149) {
				showmessage("Plate Armor", 360);
				DeleteActor(actor);
				plr.armortype = 3;
				plr.armor = 0;
				addarmor(plr, 150);
				SND_Sound(S_POTION1);
				addscore(&plr, 40);
				treasuresfound++;
			}
		});

	items[a++].Init(26, 26, true, false, [](PLAYER& plr, DWHActor* actor) // CHAINMAIL
		{
			if (plr.armor <= 99) {
				showmessage("Chain Mail", 360);
				DeleteActor(actor);
				plr.armortype = 2;
				plr.armor = 0;
				addarmor(plr, 100);
				SND_Sound(S_POTION1);
				addscore(&plr, 20);
				treasuresfound++;
			}
		});
	items[a++].Init(47, 50, false, false, [](PLAYER& plr, DWHActor* actor) // LEATHERARMOR
		{
			if (plr.armor <= 49) {
				showmessage("Leather Armor", 360);
				DeleteActor(actor);
				plr.armortype = 1;
				plr.armor = 0;
				addarmor(plr, 50);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			}
		});
	items[a++].Init(56, 49, true, false, [](PLAYER &plr, DWHActor* actor) // GIFTBOX
		{
			auto& spr = actor->s();
			spr.detail = 0;
			treasuresfound++;
			spritesound(S_TREASURE1, actor);
			int j = krand() % 8;
			switch (j) {
			case 0:
				switch (krand() % 5) {
				case 0:
					if (!potionspace(plr, 0))
						break;
					showmessage("Health Potion", 360);
					updatepotion(plr, HEALTHPOTION);
					plr.currentpotion = 0;
					SND_Sound(S_POTION1);
					addscore(&plr, 10);
					break;
				case 1:
					if (!potionspace(plr, 1))
						break;
					showmessage("Strength Potion", 360);
					updatepotion(plr, STRENGTHPOTION);
					plr.currentpotion = 1;
					SND_Sound(S_POTION1);
					addscore(&plr, 20);
					break;
				case 2:
					if (!potionspace(plr, 2))
						break;
					showmessage("Cure Poison Potion", 360);
					updatepotion(plr, ARMORPOTION);
					plr.currentpotion = 2;
					SND_Sound(S_POTION1);
					addscore(&plr, 15);
					break;
				case 3:
					if (!potionspace(plr, 3))
						break;
					showmessage("Resist Fire Potion", 360);
					updatepotion(plr, FIREWALKPOTION);
					plr.currentpotion = 3;
					SND_Sound(S_POTION1);
					addscore(&plr, 15);
					break;
				case 4:
					if (!potionspace(plr, 4))
						break;
					showmessage("Invisibility Potion", 360);
					updatepotion(plr, INVISIBLEPOTION);
					plr.currentpotion = 4;
					SND_Sound(S_POTION1);
					addscore(&plr, 30);
					break;
				}
				spr.picnum = OPENCHEST;
				break;
			case 1:
				switch (krand() % 8) {
				case 0:
					if (plr.orbammo[0] < 10) {
						plr.orb[0] = 1;
						plr.orbammo[0]++;
						showmessage("Scare Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 1:
					if (plr.orbammo[1] < 10) {
						plr.orb[1] = 1;
						plr.orbammo[1]++;
						showmessage("Night Vision Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 2:
					if (plr.orbammo[2] < 10) {
						plr.orb[2] = 1;
						plr.orbammo[2]++;
						showmessage("Freeze Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 3:
					if (plr.orbammo[3] < 10) {
						plr.orb[3] = 1;
						plr.orbammo[3]++;
						showmessage("Magic Arrow Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 4:
					if (plr.orbammo[4] < 10) {
						plr.orb[4] = 1;
						plr.orbammo[4]++;
						showmessage("Open Door Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 5:
					if (plr.orbammo[5] < 10) {
						plr.orb[5] = 1;
						plr.orbammo[5]++;
						showmessage("Fly Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 6:
					if (plr.orbammo[6] < 10) {
						plr.orb[6] = 1;
						plr.orbammo[6]++;
						showmessage("Fireball Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				case 7:
					if (plr.orbammo[7] < 10) {
						plr.orb[7] = 1;
						plr.orbammo[7]++;
						showmessage("Nuke Scroll", 360);
						SND_Sound(S_POTION1);
					}
					break;
				}
				spr.picnum = OPENCHEST;
				break;
			case 2:
				spr.picnum = OPENCHEST;
				addscore(&plr, (krand() % 400) + 100);
				showmessage("Treasure Chest", 360);
				SND_Sound(S_POTION1);
				break;
			case 3:
				// random weapon
				switch ((krand() % 5) + 1) {
				case 1:
					if (plr.ammo[1] < 12) {
						plr.weapon[1] = 1;
						plr.ammo[1] = 40;
						showmessage("Dagger", 360);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 1)
							autoweaponchange(plr, 1);
						addscore(&plr, 10);
					}
					break;
				case 2:
					if (plr.ammo[3] < 12) {
						plr.weapon[3] = 1;
						plr.ammo[3] = 55;
						showmessage("Morning Star", 360);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 3)
							autoweaponchange(plr, 3);
						addscore(&plr, 20);
					}
					break;
				case 3:
					if (plr.ammo[2] < 12) {
						plr.weapon[2] = 1;
						plr.ammo[2] = 30;
						showmessage("Short Sword", 360);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 2)
							autoweaponchange(plr, 2);
						addscore(&plr, 10);
					}
					break;
				case 4:
					if (plr.ammo[5] < 12) {
						plr.weapon[5] = 1;
						plr.ammo[5] = 100;
						showmessage("Battle axe", 360);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 5)
							autoweaponchange(plr, 5);
						addscore(&plr, 30);
					}
					break;
				case 5:
					if (plr.weapon[7] == 1) {
						plr.weapon[7] = 2;
						plr.ammo[7] = 1;
						showmessage("Pike axe", 360);
						DeleteActor(actor);
						SND_Sound(S_POTION1);
						addscore(&plr, 30);
					}
					if (plr.weapon[7] == 2) {
						plr.weapon[7] = 2;
						plr.ammo[7]++;
						showmessage("Pike axe", 360);
						DeleteActor(actor);
						SND_Sound(S_POTION1);
						addscore(&plr, 30);
					}
					if (plr.weapon[7] < 1) {
						if (plr.ammo[7] < 12) {
							plr.weapon[7] = 1;
							plr.ammo[7] = 30;
							showmessage("Pike axe", 360);
							DeleteActor(actor);
							SND_Sound(S_POTION1);
							if (plr.selectedgun < 7)
								autoweaponchange(plr, 7);
							addscore(&plr, 30);
						}
					}
					break;
				}
				spr.picnum = OPENCHEST;
				break;
			case 4:
				// random armor
				switch (krand() & 4) {
				case 0:
					showmessage("Hero Time", 360);
					addarmor(plr, 10);
					plr.helmettime = 7200;
					SND_Sound(S_STING1 + krand() % 2);
					break;
				case 1:
					if (plr.armor <= 149) {
						showmessage("Plate Armor", 360);
						plr.armortype = 3;
						plr.armor = 0;
						addarmor(plr, 150);
						SND_Sound(S_POTION1);
						addscore(&plr, 40);
					}
					break;
				case 2:
					if (plr.armor <= 99) {
						showmessage("Chain Mail", 360);
						plr.armortype = 2;
						plr.armor = 0;
						addarmor(plr, 100);
						SND_Sound(S_POTION1);
						addscore(&plr, 20);
					}
					break;
				case 3:
					if (plr.armor <= 49) {
						showmessage("Leather Armor", 360);
						plr.armortype = 1;
						plr.armor = 0;
						addarmor(plr, 50);
						SND_Sound(S_POTION1);
						addscore(&plr, 20);
					}
					break;
				}
				spr.picnum = OPENCHEST;
				break;
			case 5:
				// poison chest
				if ((krand() & 2) == 0) {
					plr.poisoned = 1;
					plr.poisontime = 7200;
					spr.detail = GIFTBOXTYPE;
					addhealth(plr, -10);
					showmessage("Poisoned Chest", 360);
				} else {
					DeleteActor(actor);
					addscore(&plr, (krand() & 400) + 100);
					showmessage("Treasure Chest", 360);
					SND_Sound(S_POTION1);
				}
				break;
			case 6:
				for (j = 0; j < 8; j++)
					explosion(actor, spr.x, spr.y, spr.z, 0);
				spritesound(S_EXPLODE, actor);
				DeleteActor(actor);
				break;
			default:
				spr.picnum = OPENCHEST;
				addscore(&plr, (krand() % 400) + 100);
				showmessage("Experience Gained", 360);
				SND_Sound(S_POTION1);
				break;
			}
		});

		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // FLASKBLUE
			{
				if (!potionspace(plr, 0))
					return;
				showmessage("Health Potion", 360);
				updatepotion(plr, HEALTHPOTION);
				plr.currentpotion = 0;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // FLASKGREEN
			{
				if (!potionspace(plr, 1))
					return;
				showmessage("Strength Potion", 360);
				updatepotion(plr, STRENGTHPOTION);
				plr.currentpotion = 1;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // FLASKOCHRE

			{
				if (!potionspace(plr, 2))
					return;
				showmessage("Cure Poison Potion", 360);
				updatepotion(plr, ARMORPOTION);
				plr.currentpotion = 2;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // FLASKRED
			{
				if (!potionspace(plr, 3))
					return;
				showmessage("Resist Fire Potion", 360);
				updatepotion(plr, FIREWALKPOTION);
				plr.currentpotion = 3;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 20);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // FLASKTAN
			{
				if (!potionspace(plr, 4))
					return;
				showmessage("Invisibility Potion", 360);
				updatepotion(plr, INVISIBLEPOTION);
				plr.currentpotion = 4;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 30);
				treasuresfound++;
			});
		items[a++].Init(14, 14, true, false, [](PLAYER& plr, DWHActor* actor) // DIAMONDRING
			{
				plr.treasure[TDIAMONDRING] = 1;
				showmessage("DIAMOND RING", 360);
				plr.armor = 0;
				addarmor(plr, 200);
				plr.armortype = 3;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 25);
				treasuresfound++;
			});
		items[a++].Init(30, 23, true, false, [](PLAYER& plr, DWHActor* actor) // SHADOWAMULET
			{
				plr.treasure[TSHADOWAMULET] = 1;
				showmessage("SHADOW AMULET", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				plr.shadowtime = 7500;
				addscore(&plr, 50);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, DWHActor* actor) // GLASSSKULL
			{
				plr.treasure[TGLASSSKULL] = 1;
				showmessage("GLASS SKULL", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				treasuresfound++;
				switch (plr.lvl) {
				case 1:
					plr.score = 2300;
					break;
				case 2:
					plr.score = 4550;
					break;
				case 3:
					plr.score = 9050;
					break;
				case 4:
					plr.score = 18050;
					break;
				case 5:
					plr.score = 36050;
					break;
				case 6:
					plr.score = 75050;
					break;
				case 7:
					plr.score = 180500;
					break;
				case 8:
					plr.score = 280500;
					break;
				}
				addscore(&plr, 10);
			});
		items[a++].Init(51, 54, true, false, [](PLAYER& plr, DWHActor* actor) // AHNK
			{
				plr.treasure[TAHNK] = 1;
				showmessage("ANKH", 360);
				plr.health = 0;
				addhealth(plr, 250);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 100);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, DWHActor* actor) // BLUESCEPTER
			{
				plr.treasure[TBLUESCEPTER] = 1;
				showmessage("Water walk scepter", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, DWHActor* actor) // YELLOWSCEPTER

			{
				plr.treasure[TYELLOWSCEPTER] = 1;
				showmessage("Fire walk scepter", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(14, 14, true, false, [](PLAYER& plr, DWHActor* actor) // ADAMANTINERING

			{
				// ring or protection +5
				plr.treasure[TADAMANTINERING] = 1;
				showmessage("ADAMANTINE RING", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 30);
				treasuresfound++;
			});
		items[a++].Init(42, 28, true, false, [](PLAYER& plr, DWHActor* actor) // ONYXRING

			{
				// protection from missile
				// anit-missile for level only
				// dont forget to cleanup values
				plr.treasure[TONYXRING] = 1;
				showmessage("ONYX RING", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 35);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, true, [](PLAYER& plr, DWHActor* actor) // PENTAGRAM

			{
				if (plr.Sector()->lotag == 4002)
					return;
				else {
					plr.treasure[TPENTAGRAM] = 1;
					showmessage("PENTAGRAM", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
				addscore(&plr, 100);
			});
		items[a++].Init(64, 64, true, false, [](PLAYER& plr, DWHActor* actor) // CRYSTALSTAFF

			{
				plr.treasure[TCRYSTALSTAFF] = 1;
				showmessage("CRYSTAL STAFF", 360);
				plr.health = 0;
				addhealth(plr, 250);
				plr.armortype = 2;
				plr.armor = 0;
				addarmor(plr, 300);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				treasuresfound++;
				addscore(&plr, 150);
			});
		items[a++].Init(26, 28, true, false, [](PLAYER& plr, DWHActor* actor) // AMULETOFTHEMIST

			{
				plr.treasure[TAMULETOFTHEMIST] = 1;
				showmessage("AMULET OF THE MIST", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				plr.invisibletime = 3200;
				addscore(&plr, 75);
				treasuresfound++;
			});
		items[a++].Init(64, 64, true, false, [](PLAYER& plr, DWHActor* actor) // HORNEDSKULL

			{
				if (isWh2()) {
					CompleteLevel(nullptr);
					return;
				}
				plr.treasure[THORNEDSKULL] = 1;
				showmessage("HORNED SKULL", 360);
				DeleteActor(actor);
				SND_Sound(S_STING2);
				addscore(&plr, 750);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, DWHActor* actor) // THEHORN

			{
				plr.treasure[TTHEHORN] = 1;
				showmessage("Ornate Horn", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				plr.vampiretime = 7200;
				// gain 5-10 hp when you kill something
				// for 60 seconds
				addscore(&plr, 350);
				treasuresfound++;
			});
		items[a++].Init(30, 20, true, false, [](PLAYER& plr, DWHActor* actor) // SAPHIRERING

			{
				plr.treasure[TSAPHIRERING] = 1;
				showmessage("SAPPHIRE RING", 360);
				plr.armortype = 3;
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 25);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, DWHActor* actor) // BRASSKEY

			{
				plr.treasure[TBRASSKEY] = 1;
				showmessage("BRASS KEY", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, DWHActor* actor) // BLACKKEY

			{
				plr.treasure[TBLACKKEY] = 1;
				showmessage("BLACK KEY", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, DWHActor* actor) // GLASSKEY

			{
				plr.treasure[TGLASSKEY] = 1;
				showmessage("GLASS KEY", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, DWHActor* actor) // IVORYKEY

			{
				plr.treasure[TIVORYKEY] = 1;
				showmessage("IVORY KEY", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLSCARE

			{
				if (plr.orbammo[0] < 10) {
					plr.orb[0] = 1;
					plr.orbammo[0]++;
					changebook(plr, 0);
					showmessage("Scare Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLNIGHT

			{
				if (plr.orbammo[1] < 10) {
					plr.orb[1] = 1;
					plr.orbammo[1]++;
					changebook(plr, 1);
					showmessage("Night Vision Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLFREEZE

			{
				if (plr.orbammo[2] < 10) {
					plr.orb[2] = 1;
					plr.orbammo[2]++;
					changebook(plr, 2);
					showmessage("Freeze Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLMAGIC

			{
				if (plr.orbammo[3] < 10) {
					plr.orb[3] = 1;
					plr.orbammo[3]++;
					changebook(plr, 3);
					showmessage("Magic Arrow Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLOPEN

			{
				if (plr.orbammo[4] < 10) {
					plr.orb[4] = 1;
					plr.orbammo[4]++;
					changebook(plr, 4);
					showmessage("Open Door Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLFLY

			{
				if (plr.orbammo[5] < 10) {
					plr.orb[5] = 1;
					plr.orbammo[5]++;
					changebook(plr, 5);
					showmessage("Fly Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLFIREBALL

			{
				if (plr.orbammo[6] < 10) {
					plr.orb[6] = 1;
					plr.orbammo[6]++;
					changebook(plr, 6);
					showmessage("Fireball Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, DWHActor* actor) // SCROLLNUKE

			{
				if (plr.orbammo[7] < 10) {
					plr.orb[7] = 1;
					plr.orbammo[7]++;
					changebook(plr, 7);
					showmessage("Nuke Scroll", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(27, 27, false, false, [](PLAYER& plr, DWHActor* actor) // QUIVER

			{
				if (plr.ammo[6] < 100) {
					plr.ammo[6] += 20;
					if (plr.ammo[6] > 100)
						plr.ammo[6] = 100;
					showmessage("Quiver of magic arrows", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WALLBOW BOW

			{
				plr.weapon[6] = 1;
				plr.ammo[6] += 10;
				if (plr.ammo[6] > 100)
					plr.ammo[6] = 100;
				showmessage("Magic bow", 360);
				DeleteActor(actor);
				SND_Sound(S_POTION1);
				if (plr.selectedgun < 6)
					autoweaponchange(plr, 6);
				addscore(&plr, 10);
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON1

			{
				if (plr.ammo[1] < 12) {
					plr.weapon[1] = 1;
					plr.ammo[1] = 40;
					showmessage("Dagger", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 1)
						autoweaponchange(plr, 1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON1A

			{
				if (plr.ammo[1] < 12) {
					plr.weapon[1] = 3;
					plr.ammo[1] = 80;
					showmessage("Jeweled Dagger", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 1);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, DWHActor* actor) // GOBWEAPON

			{
				if (plr.ammo[2] < 12) {
					plr.weapon[2] = 1;
					plr.ammo[2] = 20;
					showmessage("Short sword", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 2)
						autoweaponchange(plr, 2);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(26, 26, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON2

			{
				if (plr.ammo[3] < 12) {
					plr.weapon[3] = 1;
					plr.ammo[3] = 55;
					showmessage("Morning Star", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 3)
						autoweaponchange(plr, 3);
					addscore(&plr, 20);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WALLSWORD WEAPON3A

			{
				if (plr.ammo[4] < 12) {
					plr.weapon[4] = 1;
					plr.ammo[4] = 160;
					showmessage("Broad Sword", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 4);
					addscore(&plr, 60);
				}
			});
		items[a++].Init(44, 39, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON3

			{
				if (plr.ammo[4] < 12) {
					plr.weapon[4] = 1;
					plr.ammo[4] = 80;
					showmessage("Broad Sword", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 4)
						autoweaponchange(plr, 4);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(25, 20, false, false, [](PLAYER& plr, DWHActor* actor) // WALLAXE WEAPON4

			{
				if (plr.ammo[5] < 12) {
					plr.weapon[5] = 1;
					plr.ammo[5] = 100;
					showmessage("Battle axe", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 5)
						autoweaponchange(plr, 5);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // THROWHALBERD

			{
				auto& spr = actor->s();
				// EG fix: don't collect moving halberds, be hurt by them as you should be
				// ...but only if you don't have the Onyx Ring
				if (spr.statnum != INACTIVE && plr.treasure[TONYXRING] == 0) {
					addhealth(plr, -((krand() % 20) + 5)); // Inflict pain
					// make it look and sound painful, too
					if ((krand() % 9) == 0) {
						spritesound(S_PLRPAIN1 + (rand() % 2), actor);
					}
					startredflash(10);
					DeleteActor(actor);
					return;
				}
				if (spr.statnum != INACTIVE && plr.treasure[TONYXRING] != 0) {
					// Can we grab?
					if (plr.ammo[9] < 12 && plr.weapon[9] != 3) {
						// You grabbed a halberd out of midair, so go ahead and be smug about it
						if ((rand() % 10) > 6)
							SND_Sound(S_PICKUPAXE);
						// fall through to collect
					}
					else {
						// Can't grab, so just block getting hit
						DeleteActor(actor);
						return;
					}
				}

				if (plr.ammo[9] < 12) {
					plr.weapon[9] = 1;
					plr.ammo[9] = 30;
					showmessage("Halberd", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 9)
						autoweaponchange(plr, 9);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON5

			{
				if (plr.ammo[9] < 12) {
					plr.weapon[9] = 1;
					plr.ammo[9] = 30;
					showmessage("Halberd", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 9)
						autoweaponchange(plr, 9);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(12, 12, false, false, [](PLAYER& plr, DWHActor* actor) // GONZOBSHIELD

			{
				if (plr.shieldpoints < 100) {
					plr.shieldtype = 2;
					plr.shieldpoints = 200;
					droptheshield = false;
					DeleteActor(actor);
					showmessage("Magic Shield", 360);
					SND_Sound(S_POTION1);
					addscore(&plr, 50);
				}
			});
		items[a++].Init(26, 26, false, false, [](PLAYER& plr, DWHActor* actor) // SHIELD

			{
				if (plr.shieldpoints < 100) {
					plr.shieldpoints = 100;
					plr.shieldtype = 1;
					DeleteActor(actor);
					showmessage("Shield", 360);
					droptheshield = false; // EG 17 Oct 2017
					SND_Sound(S_POTION1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON5B

			{
				if (plr.ammo[9] < 12) { // XXX orly?
					DeleteActor(actor);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WALLPIKE

			{
				if (plr.weapon[7] == 1) {
					plr.weapon[7] = 2;
					plr.ammo[7] = 2;
					showmessage("Pike axe", 360);
					DeleteActor(actor);
					SND_Sound(S_PICKUPAXE);
					addscore(&plr, 30);
				}
				if (plr.weapon[7] == 2) {
					plr.weapon[7] = 2;
					plr.ammo[7]++;
					showmessage("Pike axe", 360);
					DeleteActor(actor);
					SND_Sound(S_PICKUPAXE);
					// score(plr, 30);
				}
				if (plr.weapon[7] < 1) {
					if (plr.ammo[7] < 12) {
						plr.weapon[7] = 1;
						plr.ammo[7] = 30;
						showmessage("Pike axe", 360);
						DeleteActor(actor);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 7)
							autoweaponchange(plr, 7);
						addscore(&plr, 30);
					}
				}
			});
		items[a++].Init(20, 15, false, true, [](PLAYER& plr, DWHActor* actor) // WEAPON6

			{
				if (plr.weapon[7] == 1) {
					plr.weapon[7] = 2;
					plr.ammo[7] = 10;
					showmessage("Pike axe", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					addscore(&plr, 30);
				}
				if (plr.weapon[7] == 2) {
					plr.weapon[7] = 2;
					plr.ammo[7] += 10;
					showmessage("Pike axe", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					// score(plr, 30);
				}
				if (plr.weapon[7] < 1) {
					if (plr.ammo[7] < 12) {
						plr.weapon[7] = 2;
						plr.ammo[7] = 10;
						showmessage("Pike axe", 360);
						DeleteActor(actor);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 7)
							autoweaponchange(plr, 7);
						addscore(&plr, 30);
					}
				}
			});
		items[a++].Init(41, 36, false, true, [](PLAYER& plr, DWHActor* actor) // WEAPON7

			{
				if (plr.ammo[8] < 12) {
					plr.weapon[8] = 1;
					plr.ammo[8] = 250;
					showmessage("Magic sword", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 8)
						autoweaponchange(plr, 8);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(32, 18, false, false, [](PLAYER& plr, DWHActor* actor) // GYSER

			{
				if (plr.manatime < 1 && plr.invincibletime <= 0 && !plr.godMode) {
					spritesound(S_FIREBALL, actor);
					addhealth(plr, -1);
					startredflash(30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // SPIKEBLADE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horizon.settarget(100);
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // SPIKE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horizon.settarget(100);
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // SPIKEPOLE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horizon.settarget(100);
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // MONSTERBALL

			{
				if (plr.manatime < 1 && plr.invincibletime <= 0 && !plr.godMode)
					addhealth(plr, -1);
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, DWHActor* actor) // WEAPON8

			{
				if (plr.ammo[8] < 12) {
					plr.weapon[8] = 1;
					plr.ammo[8] = 250;
					showmessage("Two Handed Sword", 360);
					DeleteActor(actor);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 8);
					addscore(&plr, 30);
				}
			});
};


END_WH_NS