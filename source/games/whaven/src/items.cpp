	#include "ns.h"
#include "wh.h"
#include "gamestate.h"

BEGIN_WH_NS

Item items[MAXITEMS - ITEMSBASE];
	
boolean isItemSprite(int i) {
	return (sprite[i].detail & 0xFF) >= ITEMSBASE && (sprite[i].detail & 0xFF) < MAXITEMS;
}
	
void InitItems()
{
	int a = 0;
	items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // SILVERBAG
		{
			showmessage("Silver!", 360);
			deletesprite(i);
			SND_Sound(S_POTION1);
			addscore(&plr, krand() % 100 + 10);
		});

	items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // GOLDBAG
		{
			showmessage("Gold!", 360);
			deletesprite(i);
			SND_Sound(S_POTION1);
			addscore(&plr, krand() % 100 + 10);
		});

	items[a++].Init(27, 28, true, false, [](PLAYER& plr, short i) // HELMET
		{
			showmessage("Hero Time", 360);
			deletesprite(i);
			if (!isWh2())
				addarmor(plr, 10);
			plr.helmettime = 7200;
			// JSA_DEMO3
			treasuresfound++;
			SND_Sound(S_STING1 + krand() % 2);
			addscore(&plr, 10);
		});

	items[a++].Init(26, 26, true, false, [](PLAYER& plr, short i) // PLATEARMOR
		{
			if (plr.armor <= 149) {
				showmessage("Plate Armor", 360);
				deletesprite(i);
				plr.armortype = 3;
				plr.armor = 0;
				addarmor(plr, 150);
				SND_Sound(S_POTION1);
				addscore(&plr, 40);
				treasuresfound++;
			}
		});

	items[a++].Init(26, 26, true, false, [](PLAYER& plr, short i) // CHAINMAIL
		{
			if (plr.armor <= 99) {
				showmessage("Chain Mail", 360);
				deletesprite(i);
				plr.armortype = 2;
				plr.armor = 0;
				addarmor(plr, 100);
				SND_Sound(S_POTION1);
				addscore(&plr, 20);
				treasuresfound++;
			}
		});
	items[a++].Init(47, 50, false, false, [](PLAYER& plr, short i) // LEATHERARMOR
		{
			if (plr.armor <= 49) {
				showmessage("Leather Armor", 360);
				deletesprite(i);
				plr.armortype = 1;
				plr.armor = 0;
				addarmor(plr, 50);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			}
		});
	items[a++].Init(56, 49, true, false, [](PLAYER &plr, short i) // GIFTBOX
		{
			sprite[i].detail = 0;
			treasuresfound++;
			spritesound(S_TREASURE1, &sprite[i]);
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
				sprite[i].picnum = OPENCHEST;
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
				sprite[i].picnum = OPENCHEST;
				break;
			case 2:
				sprite[i].picnum = OPENCHEST;
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
						deletesprite(i);
						SND_Sound(S_POTION1);
						addscore(&plr, 30);
					}
					if (plr.weapon[7] == 2) {
						plr.weapon[7] = 2;
						plr.ammo[7]++;
						showmessage("Pike axe", 360);
						deletesprite(i);
						SND_Sound(S_POTION1);
						addscore(&plr, 30);
					}
					if (plr.weapon[7] < 1) {
						if (plr.ammo[7] < 12) {
							plr.weapon[7] = 1;
							plr.ammo[7] = 30;
							showmessage("Pike axe", 360);
							deletesprite(i);
							SND_Sound(S_POTION1);
							if (plr.selectedgun < 7)
								autoweaponchange(plr, 7);
							addscore(&plr, 30);
						}
					}
					break;
				}
				sprite[i].picnum = OPENCHEST;
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
				sprite[i].picnum = OPENCHEST;
				break;
			case 5:
				// poison chest
				if ((krand() & 2) == 0) {
					plr.poisoned = 1;
					plr.poisontime = 7200;
					sprite[i].detail = GIFTBOXTYPE;
					addhealth(plr, -10);
					showmessage("Poisoned Chest", 360);
				} else {
					deletesprite(i);
					addscore(&plr, (krand() & 400) + 100);
					showmessage("Treasure Chest", 360);
					SND_Sound(S_POTION1);
				}
				break;
			case 6:
				for (j = 0; j < 8; j++)
					explosion(i, sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].owner);
				spritesound(S_EXPLODE, &sprite[i]);
				deletesprite(i);
				break;
			default:
				sprite[i].picnum = OPENCHEST;
				addscore(&plr, (krand() % 400) + 100);
				showmessage("Experience Gained", 360);
				SND_Sound(S_POTION1);
				break;
			}
		});

		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // FLASKBLUE
			{
				if (!potionspace(plr, 0))
					return;
				showmessage("Health Potion", 360);
				updatepotion(plr, HEALTHPOTION);
				plr.currentpotion = 0;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // FLASKGREEN
			{
				if (!potionspace(plr, 1))
					return;
				showmessage("Strength Potion", 360);
				updatepotion(plr, STRENGTHPOTION);
				plr.currentpotion = 1;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // FLASKOCHRE

			{
				if (!potionspace(plr, 2))
					return;
				showmessage("Cure Poison Potion", 360);
				updatepotion(plr, ARMORPOTION);
				plr.currentpotion = 2;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // FLASKRED
			{
				if (!potionspace(plr, 3))
					return;
				showmessage("Resist Fire Potion", 360);
				updatepotion(plr, FIREWALKPOTION);
				plr.currentpotion = 3;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 20);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // FLASKTAN
			{
				if (!potionspace(plr, 4))
					return;
				showmessage("Invisibility Potion", 360);
				updatepotion(plr, INVISIBLEPOTION);
				plr.currentpotion = 4;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 30);
				treasuresfound++;
			});
		items[a++].Init(14, 14, true, false, [](PLAYER& plr, short i) // DIAMONDRING
			{
				plr.treasure[TDIAMONDRING] = 1;
				showmessage("DIAMOND RING", 360);
				plr.armor = 0;
				addarmor(plr, 200);
				plr.armortype = 3;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 25);
				treasuresfound++;
			});
		items[a++].Init(30, 23, true, false, [](PLAYER& plr, short i) // SHADOWAMULET
			{
				plr.treasure[TSHADOWAMULET] = 1;
				showmessage("SHADOW AMULET", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				plr.shadowtime = 7500;
				addscore(&plr, 50);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, false, [](PLAYER& plr, short i) // GLASSSKULL
			{
				plr.treasure[TGLASSSKULL] = 1;
				showmessage("GLASS SKULL", 360);
				deletesprite(i);
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
		items[a++].Init(51, 54, true, false, [](PLAYER& plr, short i) // AHNK
			{
				plr.treasure[TAHNK] = 1;
				showmessage("ANKH", 360);
				plr.health = 0;
				addhealth(plr, 250);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 100);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, short i) // BLUESCEPTER
			{
				plr.treasure[TBLUESCEPTER] = 1;
				showmessage("Water walk scepter", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, short i) // YELLOWSCEPTER

			{
				plr.treasure[TYELLOWSCEPTER] = 1;
				showmessage("Fire walk scepter", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 10);
				treasuresfound++;
			});
		items[a++].Init(14, 14, true, false, [](PLAYER& plr, short i) // ADAMANTINERING

			{
				// ring or protection +5
				plr.treasure[TADAMANTINERING] = 1;
				showmessage("ADAMANTINE RING", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 30);
				treasuresfound++;
			});
		items[a++].Init(42, 28, true, false, [](PLAYER& plr, short i) // ONYXRING

			{
				// protection from missile
				// anit-missile for level only
				// dont forget to cleanup values
				plr.treasure[TONYXRING] = 1;
				showmessage("ONYX RING", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 35);
				treasuresfound++;
			});
		items[a++].Init(-1, -1, true, true, [](PLAYER& plr, short i) // PENTAGRAM

			{
				if (sector[plr.sector].lotag == 4002)
					return;
				else {
					plr.treasure[TPENTAGRAM] = 1;
					showmessage("PENTAGRAM", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
				addscore(&plr, 100);
			});
		items[a++].Init(64, 64, true, false, [](PLAYER& plr, short i) // CRYSTALSTAFF

			{
				plr.treasure[TCRYSTALSTAFF] = 1;
				showmessage("CRYSTAL STAFF", 360);
				plr.health = 0;
				addhealth(plr, 250);
				plr.armortype = 2;
				plr.armor = 0;
				addarmor(plr, 300);
				deletesprite(i);
				SND_Sound(S_POTION1);
				treasuresfound++;
				addscore(&plr, 150);
			});
		items[a++].Init(26, 28, true, false, [](PLAYER& plr, short i) // AMULETOFTHEMIST

			{
				plr.treasure[TAMULETOFTHEMIST] = 1;
				showmessage("AMULET OF THE MIST", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				plr.invisibletime = 3200;
				addscore(&plr, 75);
				treasuresfound++;
			});
		items[a++].Init(64, 64, true, false, [](PLAYER& plr, short i) // HORNEDSKULL

			{
				if (isWh2()) {
					CompleteLevel(nullptr);
					return;
				}
				plr.treasure[THORNEDSKULL] = 1;
				showmessage("HORNED SKULL", 360);
				deletesprite(i);
				SND_Sound(S_STING2);
				addscore(&plr, 750);
				treasuresfound++;
			});
		items[a++].Init(32, 32, true, false, [](PLAYER& plr, short i) // THEHORN

			{
				plr.treasure[TTHEHORN] = 1;
				showmessage("Ornate Horn", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				plr.vampiretime = 7200;
				// gain 5-10 hp when you kill something
				// for 60 seconds
				addscore(&plr, 350);
				treasuresfound++;
			});
		items[a++].Init(30, 20, true, false, [](PLAYER& plr, short i) // SAPHIRERING

			{
				plr.treasure[TSAPHIRERING] = 1;
				showmessage("SAPPHIRE RING", 360);
				plr.armortype = 3;
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 25);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, short i) // BRASSKEY

			{
				plr.treasure[TBRASSKEY] = 1;
				showmessage("BRASS KEY", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, short i) // BLACKKEY

			{
				plr.treasure[TBLACKKEY] = 1;
				showmessage("BLACK KEY", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, short i) // GLASSKEY

			{
				plr.treasure[TGLASSKEY] = 1;
				showmessage("GLASS KEY", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(24, 24, true, false, [](PLAYER& plr, short i) // IVORYKEY

			{
				plr.treasure[TIVORYKEY] = 1;
				showmessage("IVORY KEY", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				addscore(&plr, 15);
				treasuresfound++;
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLSCARE

			{
				if (plr.orbammo[0] < 10) {
					plr.orb[0] = 1;
					plr.orbammo[0]++;
					changebook(plr, 0);
					showmessage("Scare Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLNIGHT

			{
				if (plr.orbammo[1] < 10) {
					plr.orb[1] = 1;
					plr.orbammo[1]++;
					changebook(plr, 1);
					showmessage("Night Vision Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLFREEZE

			{
				if (plr.orbammo[2] < 10) {
					plr.orb[2] = 1;
					plr.orbammo[2]++;
					changebook(plr, 2);
					showmessage("Freeze Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLMAGIC

			{
				if (plr.orbammo[3] < 10) {
					plr.orb[3] = 1;
					plr.orbammo[3]++;
					changebook(plr, 3);
					showmessage("Magic Arrow Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLOPEN

			{
				if (plr.orbammo[4] < 10) {
					plr.orb[4] = 1;
					plr.orbammo[4]++;
					changebook(plr, 4);
					showmessage("Open Door Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLFLY

			{
				if (plr.orbammo[5] < 10) {
					plr.orb[5] = 1;
					plr.orbammo[5]++;
					changebook(plr, 5);
					showmessage("Fly Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLFIREBALL

			{
				if (plr.orbammo[6] < 10) {
					plr.orb[6] = 1;
					plr.orbammo[6]++;
					changebook(plr, 6);
					showmessage("Fireball Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(35, 36, true, true, [](PLAYER& plr, short i) // SCROLLNUKE

			{
				if (plr.orbammo[7] < 10) {
					plr.orb[7] = 1;
					plr.orbammo[7]++;
					changebook(plr, 7);
					showmessage("Nuke Scroll", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					treasuresfound++;
				}
			});
		items[a++].Init(27, 27, false, false, [](PLAYER& plr, short i) // QUIVER

			{
				if (plr.ammo[6] < 100) {
					plr.ammo[6] += 20;
					if (plr.ammo[6] > 100)
						plr.ammo[6] = 100;
					showmessage("Quiver of magic arrows", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WALLBOW BOW

			{
				plr.weapon[6] = 1;
				plr.ammo[6] += 10;
				if (plr.ammo[6] > 100)
					plr.ammo[6] = 100;
				showmessage("Magic bow", 360);
				deletesprite(i);
				SND_Sound(S_POTION1);
				if (plr.selectedgun < 6)
					autoweaponchange(plr, 6);
				addscore(&plr, 10);
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, short i) // WEAPON1

			{
				if (plr.ammo[1] < 12) {
					plr.weapon[1] = 1;
					plr.ammo[1] = 40;
					showmessage("Dagger", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 1)
						autoweaponchange(plr, 1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, short i) // WEAPON1A

			{
				if (plr.ammo[1] < 12) {
					plr.weapon[1] = 3;
					plr.ammo[1] = 80;
					showmessage("Jeweled Dagger", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 1);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(34, 21, false, false, [](PLAYER& plr, short i) // GOBWEAPON

			{
				if (plr.ammo[2] < 12) {
					plr.weapon[2] = 1;
					plr.ammo[2] = 20;
					showmessage("Short sword", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 2)
						autoweaponchange(plr, 2);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(26, 26, false, false, [](PLAYER& plr, short i) // WEAPON2

			{
				if (plr.ammo[3] < 12) {
					plr.weapon[3] = 1;
					plr.ammo[3] = 55;
					showmessage("Morning Star", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 3)
						autoweaponchange(plr, 3);
					addscore(&plr, 20);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WALLSWORD WEAPON3A

			{
				if (plr.ammo[4] < 12) {
					plr.weapon[4] = 1;
					plr.ammo[4] = 160;
					showmessage("Broad Sword", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 4);
					addscore(&plr, 60);
				}
			});
		items[a++].Init(44, 39, false, false, [](PLAYER& plr, short i) // WEAPON3

			{
				if (plr.ammo[4] < 12) {
					plr.weapon[4] = 1;
					plr.ammo[4] = 80;
					showmessage("Broad Sword", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 4)
						autoweaponchange(plr, 4);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(25, 20, false, false, [](PLAYER& plr, short i) // WALLAXE WEAPON4

			{
				if (plr.ammo[5] < 12) {
					plr.weapon[5] = 1;
					plr.ammo[5] = 100;
					showmessage("Battle axe", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 5)
						autoweaponchange(plr, 5);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // THROWHALBERD

			{
				// EG fix: don't collect moving halberds, be hurt by them as you should be
				// ...but only if you don't have the Onyx Ring
				if (sprite[i].statnum != INACTIVE && plr.treasure[TONYXRING] == 0) {
					addhealth(plr, -((krand() % 20) + 5)); // Inflict pain
					// make it look and sound painful, too
					if ((krand() % 9) == 0) {
						spritesound(S_PLRPAIN1 + (rand() % 2), &sprite[i]);
					}
					startredflash(10);
					deletesprite(i);
					return;
				}
				if (sprite[i].statnum != INACTIVE && plr.treasure[TONYXRING] != 0) {
					// Can we grab?
					if (plr.ammo[9] < 12 && plr.weapon[9] != 3) {
						// You grabbed a halberd out of midair, so go ahead and be smug about it
						if ((rand() % 10) > 6)
							SND_Sound(S_PICKUPAXE);
						// fall through to collect
					}
					else {
						// Can't grab, so just block getting hit
						deletesprite(i);
						return;
					}
				}

				if (plr.ammo[9] < 12) {
					plr.weapon[9] = 1;
					plr.ammo[9] = 30;
					showmessage("Halberd", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 9)
						autoweaponchange(plr, 9);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WEAPON5

			{
				if (plr.ammo[9] < 12) {
					plr.weapon[9] = 1;
					plr.ammo[9] = 30;
					showmessage("Halberd", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 9)
						autoweaponchange(plr, 9);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(12, 12, false, false, [](PLAYER& plr, short i) // GONZOBSHIELD

			{
				if (plr.shieldpoints < 100) {
					plr.shieldtype = 2;
					plr.shieldpoints = 200;
					droptheshield = false;
					deletesprite(i);
					showmessage("Magic Shield", 360);
					SND_Sound(S_POTION1);
					addscore(&plr, 50);
				}
			});
		items[a++].Init(26, 26, false, false, [](PLAYER& plr, short i) // SHIELD

			{
				if (plr.shieldpoints < 100) {
					plr.shieldpoints = 100;
					plr.shieldtype = 1;
					deletesprite(i);
					showmessage("Shield", 360);
					droptheshield = false; // EG 17 Oct 2017
					SND_Sound(S_POTION1);
					addscore(&plr, 10);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WEAPON5B

			{
				if (plr.ammo[9] < 12) { // XXX orly?
					deletesprite(i);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WALLPIKE

			{
				if (plr.weapon[7] == 1) {
					plr.weapon[7] = 2;
					plr.ammo[7] = 2;
					showmessage("Pike axe", 360);
					deletesprite(i);
					SND_Sound(S_PICKUPAXE);
					addscore(&plr, 30);
				}
				if (plr.weapon[7] == 2) {
					plr.weapon[7] = 2;
					plr.ammo[7]++;
					showmessage("Pike axe", 360);
					deletesprite(i);
					SND_Sound(S_PICKUPAXE);
					// score(plr, 30);
				}
				if (plr.weapon[7] < 1) {
					if (plr.ammo[7] < 12) {
						plr.weapon[7] = 1;
						plr.ammo[7] = 30;
						showmessage("Pike axe", 360);
						deletesprite(i);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 7)
							autoweaponchange(plr, 7);
						addscore(&plr, 30);
					}
				}
			});
		items[a++].Init(20, 15, false, true, [](PLAYER& plr, short i) // WEAPON6

			{
				if (plr.weapon[7] == 1) {
					plr.weapon[7] = 2;
					plr.ammo[7] = 10;
					showmessage("Pike axe", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					addscore(&plr, 30);
				}
				if (plr.weapon[7] == 2) {
					plr.weapon[7] = 2;
					plr.ammo[7] += 10;
					showmessage("Pike axe", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					// score(plr, 30);
				}
				if (plr.weapon[7] < 1) {
					if (plr.ammo[7] < 12) {
						plr.weapon[7] = 2;
						plr.ammo[7] = 10;
						showmessage("Pike axe", 360);
						deletesprite(i);
						SND_Sound(S_POTION1);
						if (plr.selectedgun < 7)
							autoweaponchange(plr, 7);
						addscore(&plr, 30);
					}
				}
			});
		items[a++].Init(41, 36, false, true, [](PLAYER& plr, short i) // WEAPON7

			{
				if (plr.ammo[8] < 12) {
					plr.weapon[8] = 1;
					plr.ammo[8] = 250;
					showmessage("Magic sword", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					if (plr.selectedgun < 8)
						autoweaponchange(plr, 8);
					addscore(&plr, 30);
				}
			});
		items[a++].Init(32, 18, false, false, [](PLAYER& plr, short i) // GYSER

			{
				if (plr.manatime < 1 && plr.invincibletime <= 0 && !plr.godMode) {
					spritesound(S_FIREBALL, &sprite[i]);
					addhealth(plr, -1);
					startredflash(30);
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // SPIKEBLADE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horiz = 200;
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // SPIKE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horiz = 200;
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // SPIKEPOLE

			{
				if (plr.invincibletime <= 0 && !plr.godMode && !justteleported) {
					addhealth(plr, -plr.health);
					plr.horiz = 200;
					plr.spiked = 1;
				}
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // MONSTERBALL

			{
				if (plr.manatime < 1 && plr.invincibletime <= 0 && !plr.godMode)
					addhealth(plr, -1);
			});
		items[a++].Init(-1, -1, false, false, [](PLAYER& plr, short i) // WEAPON8

			{
				if (plr.ammo[8] < 12) {
					plr.weapon[8] = 1;
					plr.ammo[8] = 250;
					showmessage("Two Handed Sword", 360);
					deletesprite(i);
					SND_Sound(S_POTION1);
					autoweaponchange(plr, 8);
					addscore(&plr, 30);
				}
			});
};


END_WH_NS