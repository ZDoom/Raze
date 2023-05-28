class SWActor : CoreActor native
{
	// all ANIMATORs.
	native int DoBunnyMove();
	native int DoBunnyGrowUp();
	native int DoBunnyEat();
	native int DoBunnyScrew();
	native int NullBunny();
	native int InitBunnySlash();
	native int DoBunnyStandKill();
	native int DoBunnyPain();
	native int DoBunnyMoveJump();
	native int DoBunnyBeginJumpAttack();
	native int BunnySpew();

	native int DoCoolgMove();
	native int NullCoolg();
	native int InitCoolgBash();
	native int InitCoolgFire();
	native int DoCoolgPain();
	native int DoCoolgDeath();
	native int DoCoolgBirth();

	native int DoCoolieMove();
	native int CooliePain();
	native int NullCoolie();
	native int SpawnCoolieExp();
	native int DoCoolieWaitBirth();
	native int SpawnCoolg();

	native int DoEelMove();
	native int InitEelFire();
	native int NullEel();
	native int DoEelDeath();

	native int NullGirlNinja();
	native int DoGirlNinjaMove();
	native int DoGirlNinjaPain();
	native int DoGirlNinjaSpecial();

	native int InitEnemyMine();
	native int InitEnemyCrossbow();

	native int DoGoroMove();
	native int NullGoro();
	native int InitGoroChop();
	native int DoGoroPain();
	native int InitEnemyFireball();

	native int DoHornetMove();
	native int DoHornetDeath();

	native int BloodSprayFall();
	native int DoSuicide();
	native int DoRadiationCloud();
	native int DoChemBomb();
	native int DoCaltrops();
	native int DoCaltropsStick();
	native int DoCarryFlag();
	native int DoCarryFlagNoDet();
	native int DoFlag();
	native int DoPhosphorus();
	native int DoBloodSpray();
	native int DoWallBloodDrip();

	native int DoLavaMove();
	native int NullLava();
	native int InitLavaThrow();
	native int InitLavaFlame();

	native int DoActorDeathMove();
	native int QueueFloorBlood();
	native int DoActorDebris();
	native int InitActorDecide();

	native int DoToiletGirl();
	native int ToiletGirlPain();
	native int ToiletGirlUzi();
	native int InitEnemyUzi();
	native int DoWashGirl();
	native int WashGirlUzi();
	native int DoTrashCan();
	native int TrashCanPain();
	native int PachinkoLightOperate();
	native int Pachinko1Operate();
	native int PachinkoCheckWin();
	native int DoCarGirl();
	native int CarGirlPain();
	native int CarGirlUzi();
	native int DoMechanicGirl();
	native int MechanicGirlPain();
	native int MechanicGirlDrill();
	native int DoSailorGirl();
	native int SailorGirlPain();
	native int SailorGirlThrow();
	native int DoPruneGirl();
	native int PruneGirlPain();
	native int WashGirlPain();

	native int DoNinjaMove();
	native int NullNinja();
	native int DoNinjaCeiling();
	native int DoNinjaPain();
	native int InitEnemyStar();
	native int InitEnemyMirv();
	native int InitEnemyNapalm();
	native int InitEnemyRocket();
	native int InitSpriteGrenade();
	native int InitFlashBomb();
	native int CheckFire();
	native int DoNinjaSpecial();
	native int DoNinjaGrabThroat();
	native int DoNinjaHariKari();

	native int DoRipperMove();
	native int NullRipper();
	native int InitRipperSlash();
	native int DoRipperStandHeart();
	native int DoRipperHang();
	native int DoRipperPain();
	native int DoRipperMoveJump();
	native int DoRipperBeginJumpAttack();
	native int DoRipperHangJF();

	native int DoRipper2Move();
	native int NullRipper2();
	native int DoRipper2Hang();
	native int DoRipper2Pain();
	native int DoRipper2MoveJump();
	native int DoRipper2BeginJumpAttack();
	native int DoRipper2HangJF();
	native int DoRipper2StandHeart();
	native int ChestRipper2();

	native int DoSerpMove();
	native int NullSerp();
	native int InitSerpSlash();
	native int InitSerpRing();
	native int InitSerpSpell();
	native int InitSerpMonstSpell();
	native int DoDeathSpecial();

	native int DoSkelMove();
	native int NullSkel();
	native int InitSkelSlash();
	native int InitSkelSpell();
	native int DoSkelPain();
	native int DoSkelInitTeleport();
	native int DoSkelTeleport();
	native int DoSkelTermTeleport();

	native int DoSkullWait();
	native int DoSerpRing();
	native int DoSkullJump();
	native int DoDamageTest();
	native int DoSkullSpawnShrap();
	native int DoBettyWait();
	native int DoBettyJump();

	native int DoSumoMove();
	native int NullSumo();
	native int InitSumoFart();
	native int InitSumoClap();
	native int InitSumoStomp();
	native int DoSumoDeathMelt();

	native int DoDefaultStat();
	native int DoPuff();
	native int DoRailPuff();
	native int DoTracer();
	native int DoEMP();
	native int DoEMPBurst();
	native int DoFastShrapJumpFall();

	native int DoTankShell();
	native int DoVehicleSmoke();
	native int DoWaterSmoke();
	native int DoUziSmoke();
	native int DoShotgunSmoke();
	native int DoUziBullet();
	native int DoBubble();
	native int DoCrossBolt();
	native int DoStar();
	native int DoLavaBoulder();
	native int DoShrapDamage();
	native int DoVulcanBoulder();
	native int DoGrenade();
	native int DoMineStuck();
	native int DoMine();
	native int DoMineSpark();
	native int DoMeteor();
	native int DoMirvMissile();
	native int DoSerpMeteor();
	native int DoSpear();
	native int DoRocket();
	native int DoRail();
	native int DoLaser();
	native int DoMicro();
	native int DoMicroMini();
	native int DoBoltThinMan();
	native int DoBoltSeeker();
	native int DoBoltFatMan();
	native int DoBoltShrapnel();
	native int DoCoolgFire();
	native int DoCoolgDrip();
	native int DoPlasma();
	native int DoShrapJumpFall();

	native int DoTracerShrap();
	native int DoVomitSplash();
	native int DoVomit();
	native int DoMirv();
	native int DoBloodWorm();
	native int DoNapalm();
	native int DoRing();
	native int DoFireball();
	native int DoBreakFlames();
	native int DoFireballFlames();
	native int DoSectorExp();
	native int SpawnShrapX();
	native int DoExpDamageTest();
	native int DoMineExpMine();
	native int DoMineExp();
	native int SpawnGrenadeSmallExp();
	native int DoElectro();
	native int DoTeleRipper();
	native int DoPlasmaDone();
	native int DoPlasmaFountain();
	native int DoFootPrints();
	native int DoPlayerSpriteReset();

	native int DoFloorBlood();
	native int DoWallBlood();

	native int DoGet();
	native int DoCoin();
	native int DoFireFly();

	native int DoZillaMove();
	native int DoZillaStomp();
	native int NullZilla();
	native int InitZillaRail();
	native int InitZillaRocket();
	native int DoZillaDeathMelt();

	native int DoZombieMove();
	native int NullZombie();
	native int DoZombiePain();
	native int InitEnemyNuke();
	native int InitEnemyRail();

	native int InitActorRunAway();
	native int InitActorAttack();
	native int InitActorDuck();
	native int InitActorEvade();
	native int InitActorFindPlayer();
	native int InitActorMoveCloser();
	native int InitActorReposition();
	native int InitActorWanderAround();
	native int InitCoolgCircle();
	native int InitCoolieCharge();
	native int InitHornetCircle();
	native int InitHornetSting();
	native int InitRipper2Charge();
	native int InitRipper2Hang();
	native int InitRipperHang();
	native int InitActorRunToward();
	native int InitActorSetDecide();
	native int DoActorDecide();
	native int DoActorMoveJump();
	native int DoActorDuck();
	native int NinjaJumpActionFunc();
	native int DoActorMoveCloser();
	native int DoActorAttack();
	native int DoActorReposition();
	native int DoCoolgCircle();
	native int DoHornetCircle();
	native int GenerateDrips();
	native int DoSpawnSpot();
	native int DoGrating();
	native int DoVator();
	native int DoVatorAuto();
	native int DoRotator();
	native int DoActorPause();
	native int DoSlidor();
	native int DoSpike();
	native int DoSpikeAuto();
	native int DoLavaErupt();
	native int SpawnVehicleSmoke();
	native int DoLaserStart();
	native int DoTracerStart();
	native int DoRailStart();
	native int DoGenerateSewerDebris();

}
