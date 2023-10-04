class BloodDudeBase : Bloodactor
{
	override int getRespawnTime()
	{
		if (!self.hasX) return -1;
		if (self.xspr.respawn == 2 || (self.xspr.respawn != 1 && gGameOptions.nMonsterSettings == Blood.MONSTERSETTINGS_2))
			return gGameOptions.nMonsterRespawnTime;
		return -1;
	}
}

class BloodPlayerBase : BloodDudeBase
{
	override int getRespawnTime()
	{
		return -1;	// no respawn for players.
	}
}

class BloodDudeCultistTommy : BloodDudeBase {}
class BloodDudeCultistShotgun : BloodDudeBase {}
class BloodDudeZombieAxeNormal : BloodDudeBase {}
class BloodDudeZombieButcher : BloodDudeBase {}
class BloodDudeZombieAxeBuried : BloodDudeBase {}
class BloodDudeGargoyleFlesh : BloodDudeBase {}
class BloodDudeGargoyleStone : BloodDudeBase {}
class BloodDudeGargoyleStatueFlesh : BloodDudeBase {}
class BloodDudeGargoyleStatueStone : BloodDudeBase {}
class BloodDudePhantasm : BloodDudeBase {}
class BloodDudeHellHound : BloodDudeBase {}
class BloodDudeHand : BloodDudeBase {}
class BloodDudeSpiderBrown : BloodDudeBase {}
class BloodDudeSpiderRed : BloodDudeBase {}
class BloodDudeSpiderBlack : BloodDudeBase {}
class BloodDudeSpiderMother : BloodDudeBase {}
class BloodDudeGillBeast : BloodDudeBase {}
class BloodDudeBoneEel : BloodDudeBase {}
class BloodDudeBat : BloodDudeBase {}
class BloodDudeRat : BloodDudeBase {}
class BloodDudePodGreen : BloodDudeBase {}
class BloodDudeTentacleGreen : BloodDudeBase {}
class BloodDudePodFire : BloodDudeBase {}
class BloodDudeTentacleFire : BloodDudeBase {}
class BloodDudePodMother : BloodDudeBase {}
class BloodDudeTentacleMother : BloodDudeBase {}
class BloodDudeCerberusTwoHead : BloodDudeBase {}
class BloodDudeCerberusOneHead : BloodDudeBase {}
class BloodDudeTchernobog : BloodDudeBase {}
class BloodDudeCultistTommyProne : BloodDudeBase {}
class BloodDudePlayer1 : BloodPlayerBase {}
class BloodDudePlayer2 : BloodPlayerBase {}
class BloodDudePlayer3 : BloodPlayerBase {}
class BloodDudePlayer4 : BloodPlayerBase {}
class BloodDudePlayer5 : BloodPlayerBase {}
class BloodDudePlayer6 : BloodPlayerBase {}
class BloodDudePlayer7 : BloodPlayerBase {}
class BloodDudePlayer8 : BloodPlayerBase {}
class BloodDudeBurningInnocent : BloodDudeBase {}
class BloodDudeBurningCultist : BloodDudeBase {}
class BloodDudeBurningZombieAxe : BloodDudeBase {}
class BloodDudeBurningZombieButcher : BloodDudeBase {}
class BloodDudeCultistReserved : BloodDudeBase {} // unused
class BloodDudeZombieAxeLaying : BloodDudeBase {}
class BloodDudeInnocent : BloodDudeBase {}
class BloodDudeCultistShotgunProne : BloodDudeBase {}
class BloodDudeCultistTesla : BloodDudeBase {}
class BloodDudeCultistTNT : BloodDudeBase {}
class BloodDudeCultistBeast : BloodDudeBase {}
class BloodDudeTinyCaleb : BloodDudeBase {}
class BloodDudeBeast : BloodDudeBase {}
class BloodDudeBurningTinyCaleb : BloodDudeBase {}
class BloodDudeBurningBeast : BloodDudeBase {}
