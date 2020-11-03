#include "ns.h"
#include "wh.h"
#include "serializer.h"
#include "mmulti.h"

BEGIN_WH_NS


FSerializer& Serialize(FSerializer& arc, const char *key, SwingDoor& sw, SwingDoor* def)
{
	if (arc.BeginObject(key))
	{
		arc.Array("wall", sw.wall, 8)
			("sector", sw.sector)
			("angopen", sw.angopen)
			("angclosed", sw.angclosed)
			("angopendir", sw.angopendir)
			("ang", sw.ang)
			("anginc", sw.anginc)
			.Array("x", sw.x, 8)
			.Array("y", sw.y, 8)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, PLAYER& sw, PLAYER* def)
{
	if (arc.BeginObject(key))
	{
		arc("x", sw.x)
			("y", sw.y)
			("z", sw.z)
			("ang", sw.ang)
			("horizon", sw.horizon)
			("height", sw.height)
			("hvel", sw.hvel)
			("sector", sw.sector)
			("oldsector", sw.oldsector)
			("spritenum", sw.spritenum)
			("keytoggle", sw.keytoggle)
			("flags", sw.flags)
			.Array("weapon", sw.weapon, countof(sw.weapon))
			.Array("preenchantedweapon", sw.preenchantedweapon, countof(sw.preenchantedweapon))
			.Array("ammo", sw.ammo, countof(sw.ammo))
			.Array("preenchantedammo", sw.preenchantedammo, countof(sw.preenchantedammo))
			.Array("orbammo", sw.orbammo, countof(sw.orbammo))
			.Array("treasure", sw.treasure, countof(sw.treasure))
			.Array("orbactive", sw.orbactive, countof(sw.orbactive))
			.Array("orb", sw.orb, countof(sw.orb))
			.Array("potion", sw.potion, countof(sw.potion))
			("lvl", sw.lvl)
			("score", sw.score)
			("health", sw.health)
			("maxhealth", sw.maxhealth)
			("armor", sw.armor)
			("armortype", sw.armortype)
			("onsomething", sw.onsomething)
			("fallz", sw.fallz)
			("dead", sw.dead)
			("turnaround", sw.turnAround)
			("shadowtime", sw.shadowtime)
			("helmettime", sw.helmettime)
			("scoretime", sw.scoretime)
			("vampiretime", sw.vampiretime)
			("selectedgun", sw.selectedgun)
			("currweapon", sw.currweapon)
			("currweapontics", sw.currweapontics)
			("currweaponanim", sw.currweaponanim)
			("currweaponframe", sw.currweaponframe)
			("currweaponfired", sw.currweaponfired)
			("currweaponattackstyle", sw.currweaponattackstyle)
			("currweaponflip", sw.currweaponflip)
			("hasshot", sw.hasshot)
			("currentpotion", sw.currentpotion)
			("strongtime", sw.strongtime)
			("invisibletime", sw.invisibletime)
			("manatime", sw.manatime)
			("orbshot", sw.orbshot)
			("spellbooktics", sw.spellbooktics)
			("spellbook", sw.spellbook)
			("spellbookframe", sw.spellbookframe)
			("spellbookflip", sw.spellbookflip)
			("nightglowtime", sw.nightglowtime)
			("showbook", sw.showbook)
			("showbooktype", sw.showbooktype)
			("showbookflip", sw.showbookflip)
			("showbookanim", sw.showbookanim)
			("currentorb", sw.currentorb)
			("spelltime", sw.spelltime)
			("shieldpoints", sw.shieldpoints)
			("shieldtype", sw.shieldtype)
			("poisoned", sw.poisoned)
			("poisontime", sw.poisontime)
			("shockme", sw.shockme)
			("invincibletime", sw.invincibletime)
			("spiked", sw.spiked)
			("spiketics", sw.spiketics)
			("spikeframe", sw.spikeframe)
			("currspikeframe", sw.currspikeframe)
			("godMode", sw.godMode)
			("noclip", sw.noclip)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, ANIMATION& sw, ANIMATION* def)
{
	if (arc.BeginObject(key))
	{
		arc("id", sw.id)
			("type", sw.type)
			("goal", sw.goal)
			("vel", sw.vel)
			("acc", sw.acc)
			.EndObject();
	}
	return arc;
}


void SerializeSectorData(FSerializer& arc)
{
	if (arc.BeginObject("sectordata"))
	{
		arc("skypancnt", skypancnt)
			.Array("skypanlist", skypanlist, skypancnt)
			("lavadrylandcnt", lavadrylandcnt)
			.Array("lavadrylandsector", lavadrylandsector, lavadrylandcnt)
			("dragsectorcnt", dragsectorcnt)
			.Array("dragsectorlist", dragsectorlist, dragsectorcnt)
			.Array("dragxdir", dragxdir, dragsectorcnt)
			.Array("dragydir", dragydir, dragsectorcnt)
			.Array("dragx1", dragx1, dragsectorcnt)
			.Array("dragy1", dragy1, dragsectorcnt)
			.Array("dragx2", dragx2, dragsectorcnt)
			.Array("dragy2", dragy2, dragsectorcnt)
			.Array("dragfloorz", dragfloorz, dragsectorcnt)
			("bobbingsectorcnt", bobbingsectorcnt)
			.Array("bobbingsectorlist", bobbingsectorlist, bobbingsectorcnt)
			("warpsectorcnt", warpsectorcnt)
			.Array("warpsectorlist", warpsectorlist, warpsectorcnt)
			("xpanningsectorcnt", xpanningsectorcnt)
			.Array("xpanningsectorlist", xpanningsectorlist, xpanningsectorcnt)
			("ypanningwallcnt", ypanningwallcnt)
			.Array("ypanningwalllist", ypanningwalllist, ypanningwallcnt)
			("floorpanningcnt", floorpanningcnt)
			.Array("floorpanninglist", floorpanninglist, floorpanningcnt)
			("swingcnt", swingcnt)
			.Array("swingdoor", swingdoor, swingcnt)
			("revolvecnt", revolvecnt)
			.Array("revolvesector", revolvesector, revolvecnt)
			.Array("revolveang", revolveang, revolvecnt)
			.Array("revolvepivotx", revolvepivotx, revolvecnt)
			.Array("revolvepivoty", revolvepivoty, revolvecnt)
			.Array("revolvex", &revolvex[0][0], revolvecnt * 32)
			.Array("revolvey", &revolvey[0][0], revolvecnt * 32)
			.Array("revolveclip", revolveclip, countof(revolveclip))
			("ironbarscnt", ironbarscnt)
			.Array("ironbarsector", ironbarsector, ironbarscnt)
			.Array("ironbarsgoal", ironbarsgoal, ironbarscnt)
			.Array("ironbarsgoal1", ironbarsgoal1, ironbarscnt)
			.Array("ironbarsgoal2", ironbarsgoal2, ironbarscnt)
			.Array("ironbarsdone", ironbarsdone, ironbarscnt)
			.Array("ironbarsanim", ironbarsanim, ironbarscnt)
			.EndObject();
	}
}


void SerializeStuff(FSerializer& arc)
{
	if (arc.BeginObject("stuff"))
	{
		arc("kills", kills)
			("killcnt", killcnt)
			("treasurescnt", treasurescnt)
			("treasuresfound", treasuresfound)
			("lockclock", lockclock)
			//("visibility", visibility)
			("thunderflash", thunderflash)
			("thundertine", thundertime)
			("dropshieldcnt", dropshieldcnt)
			("droptheshield", droptheshield)
			("floormirrorcnt", floormirrorcnt)
			.Array("floormirrorsector", floormirrorsector, floormirrorcnt)
			.Array("arrowsprite", arrowsprite, countof(arrowsprite))
			.Array("ceilingshadearray", ceilingshadearray, numsectors)
			.Array("floorshadearray", floorshadearray, numsectors)
			.Array("wallshadearray", wallshadearray, numwalls)
			.Array("players", player, numplayers)
			("numanimates", gAnimationCount)
			.Array("animationdata", gAnimationData, gAnimationCount)
			.EndObject();
	}
	if (arc.isReading())
	{
		for (int i = gAnimationCount - 1; i >= 0; i--)
		{
			ANIMATION& gAnm = gAnimationData[i];
			switch (gAnm.type)
			{
			case WALLX:
			case WALLY:
				//game.pInt.setwallinterpolate(gAnm.id, (WALL)object);
				break;
			case FLOORZ:
				//game.pInt.setfloorinterpolate(gAnm.id, (SECTOR)object);
				break;
			case CEILZ:
				//game.pInt.setceilinterpolate(gAnm.id, (SECTOR)object);
				break;
			}
		}

	}
}

void GameInterface::SerializeGameState(FSerializer& arc)
{
	SerializeSectorData(arc);
	SerializeStuff(arc);
}

END_WH_NS
