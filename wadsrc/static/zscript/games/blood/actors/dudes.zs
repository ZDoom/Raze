class BloodDudeBase : Bloodactor
{
	meta Name seqStartName;
	meta int seqStartID;
	meta int starthealth;
	meta int eyeHeight;
	meta int aimHeight; // used by just Cerberus
	meta double hearDist;
	meta double seeDist;
	meta double periphery;
	meta double meleeDist; // unused?
	meta int fleeHealth; // at which hp level enemy will turn in burning dude
	meta int hinderDamage; // recoil dmgcontrol
	meta int changeTarget; // chance to change target when attacked someone else
	meta int changeTargetKin; // chance to change target when attacked by same type
	meta int alertChance;
	meta int lockOut;
	meta double frontSpeed;
	meta double sideSpeed;
	meta double backSpeed;
	meta double turnRange;
	meta int gibType[3]; // which gib used when explode dude

	meta int classflags;
	meta Sound explodeSound;
	meta class<BloodDudeBase> deathMorphType;
	
	property prefix: none;
	property seqStartName: seqStartName;
	property seqStartID: seqStartID;
	property health: startHealth;
	property mass: mass;
	property eyeHeight: eyeHeight;
	property aimHeight: aimHeight;
	property hearDist: hearDist;
	property seeDist: seeDist;
	property periphery: periphery;
	property meleeDist: meleeDist;
	property fleeHealth: fleeHealth;
	property hinderDamage: hinderDamage;
	property changeTargetChance: changeTarget;
	property changeTargetKinChance: changeTargetKin;
	property alertChance: alertChance;
	property lockOut: lockOut;
	property frontSpeed: frontSpeed;
	property sideSpeed: sideSpeed;
	property backSpeed: backSpeed;
	property turnRange: turnRange;

	property explodeSound: explodeSound;
	property deathMorphType: deathMorphType;
	flagdef burning: classflags, 0;
	flagdef floorhitdamage: classflags, 1;
	
	
	default
	{
		AISTATE "genIdle", "+0", 1, 0, null, null, null, null, "none";
		AISTATE "genRecoil", "+5", 5, 20, null, null, null, null, "genIdle";
		AISTATE "batIdle", "+0", 0, 0, null, null, null, batThinkTarget, "none";
		AISTATE "batFlyIdle", "+6", 0, 0, null, null, null, batThinkTarget, "none";
		AISTATE "batChase", "+6", 4, 0, null, null, batMoveForward, batThinkChase, "batFlyIdle";
		AISTATE "batPonder", "+6", -1, 0, null, null, null, batThinkPonder, "none";
		AISTATE "batGoto", "+6", 2, 600, null, null, batMoveForward, batThinkGoto, "batFlyIdle";
		AISTATE "batBite", "+7", 4, 60, batBiteSeqCallback, null, null, null, "batPonder";
		AISTATE "batRecoil", "+5", 5, 0, null, null, null, null, "batChase";
		AISTATE "batSearch", "+6", 3, 120, null, null, batMoveForward, batThinkSearch, "batFlyIdle";
		AISTATE "batSwoop", "+6", -1, 60, null, null, batMoveSwoop, batThinkChase, "batChase";
		AISTATE "batFly", "+6", 2, 0, null, null, batMoveFly, batThinkChase, "batChase";
		AISTATE "batTurn", "+6", 2, 60, null, null, aiMoveTurn, null, "batChase";
		AISTATE "batHide", "+6", -1, 0, null, null, batMoveToCeil, batMoveForward, "none";
		AISTATE "batDodgeUp", "+6", 2, 120, null, null, batMoveDodgeUp, null, "batChase";
		AISTATE "batDodgeUpRight", "+6", 2, 90, null, null, batMoveDodgeUp, null, "batChase";
		AISTATE "batDodgeUpLeft", "+6", 2, 90, null, null, batMoveDodgeUp, null, "batChase";
		AISTATE "batDodgeDown", "+6", 2, 120, null, null, batMoveDodgeDown, null, "batChase";
		AISTATE "batDodgeDownRight", "+6", 2, 90, null, null, batMoveDodgeDown, null, "batChase";
		AISTATE "batDodgeDownLeft", "+6", 2, 90, null, null, batMoveDodgeDown, null, "batChase";
		AISTATE "beastIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "beastChase", "+8", 4, 0, null, null, beastMoveForward, beastThinkChase, "none";
		AISTATE "beastDodge", "+8", 2, 60, null, null, aiMoveDodge, null, "beastChase";
		AISTATE "beastGoto", "+8", 2, 600, null, null, beastMoveForward, beastThinkGoto, "beastIdle";
		AISTATE "beastSlash", "+6", 4, 120, SlashSeqCallback, null, null, null, "beastChase";
		AISTATE "beastStomp", "+7", 4, 120, StompSeqCallback, null, null, null, "beastChase";
		AISTATE "beastSearch", "+8", 3, 120, null, null, beastMoveForward, beastThinkSearch, "beastIdle";
		AISTATE "beastRecoil", "+5", 5, 0, null, null, null, null, "beastDodge";
		AISTATE "beastTeslaRecoil", "+4", 5, 0, null, null, null, null, "beastDodge";
		AISTATE "beastSwimIdle", "+9", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "beastSwimChase", "+9", 4, 0, null, null, beastMoveSwim, beastThinkSwimChase, "none";
		AISTATE "beastSwimDodge", "+9", 2, 90, null, null, aiMoveDodge, null, "beastSwimChase";
		AISTATE "beastSwimGoto", "+9", 2, 600, null, null, beastMoveForward, beastThinkSwimGoto, "beastSwimIdle";
		AISTATE "beastSwimSearch", "+9", 3, 120, null, null, beastMoveForward, beastThinkSearch, "beastSwimIdle";
		AISTATE "beastSwimSlash", "+9", 4, 0, SlashSeqCallback, null, null, beastThinkSwimChase, "beastSwimChase";
		AISTATE "beastSwimRecoil", "+5", 5, 0, null, null, null, null, "beastSwimDodge";
		AISTATE "beastMorphToBeast", "", 0, -1, null, MorphToBeast, null, null, "beastIdle";
		AISTATE "beastMorphFromCultist", "+2576", -1, 0, null, null, null, null, "beastMorphToBeast";
		AISTATE "beastMoveSwimChaseAlt", "+9", -1, 120, null, null, beastMoveSwimAlt, beastThinkSwimChase, "beastSwimChase";
		AISTATE "beastSwimAttack", "+9", -1, 0, null, null, beastMoveIn, beastThinkSwimChase, "beastSwimChase";
		AISTATE "beastSwimTurn", "+9", -1, 120, null, null, aiMoveTurn, null, "beastSwimChase";
		AISTATE "eelIdle", "+0", 0, 0, null, null, null, eelThinkTarget, "none";
		AISTATE "eelFlyIdle", "+0", 0, 0, null, null, null, eelThinkTarget, "none";
		AISTATE "eelChase", "+0", 4, 0, null, null, eelMoveForward, eelThinkChase, "eelIdle";
		AISTATE "eelPonder", "+0", -1, 0, null, null, null, eelThinkPonder, "none";
		AISTATE "eelGoto", "+0", 2, 600, null, null, null, eelThinkGoto, "eelIdle";
		AISTATE "eelBite", "+7", 4, 60, eelBiteSeqCallback, null, null, null, "eelChase";
		AISTATE "eelRecoil", "+5", 5, 0, null, null, null, null, "eelChase";
		AISTATE "eelSearch", "+0", 3, 120, null, null, eelMoveForward, eelThinkSearch, "eelIdle";
		AISTATE "eelSwoop", "+0", -1, 60, null, null, eelMoveSwoop, eelThinkChase, "eelChase";
		AISTATE "eelFly", "+0", 2, 0, null, null, eelMoveAscend, eelThinkChase, "eelChase";
		AISTATE "eelTurn", "+0", 2, 60, null, null, aiMoveTurn, null, "eelChase";
		AISTATE "eelHide", "+0", -1, 0, null, null, eelMoveToCeil, eelMoveForward, "none";
		AISTATE "eelDodgeUp", "+0", 2, 120, null, null, eelMoveDodgeUp, null, "eelChase";
		AISTATE "eelDodgeUpRight", "+0", 2, 90, null, null, eelMoveDodgeUp, null, "eelChase";
		AISTATE "eelDodgeUpLeft", "+0", 2, 90, null, null, eelMoveDodgeUp, null, "eelChase";
		AISTATE "eelDodgeDown", "+0", 2, 120, null, null, eelMoveDodgeDown, null, "eelChase";
		AISTATE "eelDodgeDownRight", "+0", 2, 90, null, null, eelMoveDodgeDown, null, "eelChase";
		AISTATE "eelDodgeDownLeft", "+0", 2, 90, null, null, eelMoveDodgeDown, null, "eelChase";
		AISTATE "tinycalebIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "tinycalebChase", "+6", 4, 0, null, null, aiMoveForward, calebThinkChase, "none";
		AISTATE "tinycalebDodge", "+6", 2, 90, null, null, aiMoveDodge, null, "tinycalebChase";
		AISTATE "tinycalebGoto", "+6", 2, 600, null, null, aiMoveForward, calebThinkGoto, "tinycalebIdle";
		AISTATE "tinycalebAttack", "+0", 4, 120, SeqAttackCallback, null, null, null, "tinycalebChase";
		AISTATE "tinycalebSearch", "+6", 3, 120, null, null, aiMoveForward, calebThinkSearch, "tinycalebIdle";
		AISTATE "tinycalebRecoil", "+5", 5, 0, null, null, null, null, "tinycalebDodge";
		AISTATE "tinycalebTeslaRecoil", "+4", 5, 0, null, null, null, null, "tinycalebDodge";
		AISTATE "tinycalebSwimIdle", "+10", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "tinycalebSwimChase", "+8", 4, 0, null, null, calebMoveSwimChase, calebThinkSwimChase, "none";
		AISTATE "tinycalebSwimDodge", "+8", 2, 90, null, null, aiMoveDodge, null, "tinycalebSwimChase";
		AISTATE "tinycalebSwimGoto", "+8", 2, 600, null, null, aiMoveForward, calebThinkSwimGoto, "tinycalebSwimIdle";
		AISTATE "tinycalebSwimSearch", "+8", 3, 120, null, null, aiMoveForward, calebThinkSearch, "tinycalebSwimIdle";
		AISTATE "tinycalebSwimAttack", "+10", 4, 0, SeqAttackCallback, null, null, null, "tinycalebSwimChase";
		AISTATE "tinycalebSwimRecoil", "+5", 5, 0, null, null, null, null, "tinycalebSwimDodge";
		AISTATE "tinycalebSwimUnused", "+8", -1, 120, null, null, calebSwimUnused, calebThinkSwimChase, "tinycalebSwimChase";
		AISTATE "tinycalebSwimMoveIn", "+8", -1, 0, null, null, calebSwimMoveIn, calebThinkSwimChase, "tinycalebSwimChase";
		AISTATE "tinycalebSwimTurn", "+8", -1, 120, null, null, aiMoveTurn, null, "tinycalebSwimChase";
		AISTATE "cerberusIdle", "+0", 0, 0, null, null, null, cerberusThinkTarget, "none";
		AISTATE "cerberusSearch", "+7", 3, 1800, null, null, aiMoveForward, cerberusThinkSearch, "cerberusIdle";
		AISTATE "cerberusChase", "+7", 4, 0, null, null, aiMoveForward, cerberusThinkChase, "none";
		AISTATE "cerberusRecoil", "+5", 5, 0, null, null, null, null, "cerberusSearch";
		AISTATE "cerberusTeslaRecoil", "+4", 5, 0, null, null, null, null, "cerberusSearch";
		AISTATE "cerberusGoto", "+7", 2, 600, null, null, aiMoveForward, cerberusThinkGoto, "cerberusIdle";
		AISTATE "cerberusBite", "+6", 4, 60, cerberusBiteSeqCallback, null, null, null, "cerberusChase";
		AISTATE "cerberusBurn", "+6", 4, 60, cerberusBurnSeqCallback, null, null, null, "cerberusChase";
		AISTATE "cerberus3Burn", "+6", 4, 60, cerberusBurnSeqCallback2, null, null, null, "cerberusChase";
		AISTATE "cerberus2Idle", "+0", 0, 0, null, null, null, cerberusThinkTarget, "none";
		AISTATE "cerberus2Search", "+7", 3, 1800, null, null, aiMoveForward, cerberusThinkSearch, "cerberus2Idle";
		AISTATE "cerberus2Chase", "+7", 4, 0, null, null, aiMoveForward, cerberusThinkChase, "none";
		AISTATE "cerberus2Recoil", "+5", 5, 0, null, null, null, null, "cerberus2Search";
		AISTATE "cerberus2Goto", "+7", 2, 600, null, null, aiMoveForward, cerberusThinkGoto, "cerberus2Idle";
		AISTATE "cerberus2Bite", "+6", 4, 60, cerberusBiteSeqCallback, null, null, null, "cerberus2Chase";
		AISTATE "cerberus2Burn", "+6", 4, 60, cerberusBurnSeqCallback, null, null, null, "cerberus2Chase";
		AISTATE "cerberus4Burn", "+6", 4, 60, cerberusBurnSeqCallback2, null, null, null, "cerberus2Chase";
		AISTATE "cerberusTurn1", "+7", -1, 120, null, null, aiMoveTurn, null, "cerberusChase";
		AISTATE "cerberusTurn2", "+7", -1, 120, null, null, aiMoveTurn, null, "cerberusChase";
		AISTATE "cultistIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "cultistProneIdle", "+17", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "fanaticProneIdle", "+17", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "cultistProneIdle3", "+17", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "cultistChase", "+9", 4, 0, null, null, aiMoveForward, cultThinkChase, "none";
		AISTATE "fanaticChase", "+0", 4, 0, null, null, aiMoveTurn, cultThinkChase, "none";
		AISTATE "cultistDodge", "+9", 2, 90, null, null, aiMoveDodge, null, "cultistChase";
		AISTATE "cultistGoto", "+9", 2, 600, null, null, aiMoveForward, cultThinkGoto, "cultistIdle";
		AISTATE "cultistProneChase", "+14", 4, 0, null, null, aiMoveForward, cultThinkChase, "none";
		AISTATE "cultistProneDodge", "+14", 2, 90, null, null, aiMoveDodge, null, "cultistProneChase";
		AISTATE "cultistTThrow", "+7", 4, 120, cultThrowSeqCallback, null, null, null, "cultistTFire";
		AISTATE "cultistSThrow", "+7", 4, 120, cultThrowSeqCallback, null, null, null, "cultistSFire";
		AISTATE "cultistTsThrow", "+7", 4, 120, cultThrowSeqCallback, null, null, null, "cultistTsFire";
		AISTATE "cultistDThrow", "+7", 4, 120, cultThrowSeqCallback, null, null, null, "cultistChase";
		AISTATE "cultistDThrow2", "+7", 4, 120, cultThrowSeqCallback2, null, null, null, "cultistChase";
		AISTATE "cultistDThrow3C", "+7", 4, 120, cultThrowSeqCallback3, null, null, null, "cultistIdle";
		AISTATE "cultistDThrow3B", "+7", 4, 120, cultThrowSeqCallback3, null, null, cultThinkSearch, "cultistDThrow3C";
		AISTATE "cultistDThrow3A", "+7", 4, 120, cultThrowSeqCallback3, null, null, cultThinkSearch, "cultistDThrow3B";
		AISTATE "cultistDThrow4", "+7", 4, 120, cultThrowSeqCallback3, null, null, cultThinkSearch, "cultistDThrow4";
		AISTATE "cultistSearch", "+9", 3, 1800, null, null, aiMoveForward, cultThinkSearch, "cultistIdle";
		AISTATE "cultistSFire", "+6", 4, 60, ShotSeqCallback, null, null, null, "cultistChase";
		AISTATE "cultistTFire", "+6", 4, 0, TommySeqCallback, null, aiMoveTurn, cultThinkChase, "cultistTFire";
		AISTATE "cultistTsFire", "+6", 4, 0, TeslaSeqCallback, null, aiMoveTurn, cultThinkChase, "cultistChase";
		AISTATE "cultistSProneFire", "+8", 4, 60, ShotSeqCallback, null, null, null, "cultistProneChase";
		AISTATE "cultistTProneFire", "+8", 4, 0, TommySeqCallback, null, aiMoveTurn, cultThinkChase, "cultistTProneFire";
		AISTATE "cultistTsProneFire", "+8", 4, 0, TeslaSeqCallback, null, aiMoveTurn, null, "cultistTsProneFire";
		AISTATE "cultistTsProneFireFixed", "+0", 8, 0, TeslaSeqCallback, null, aiMoveTurn, null, "cultistTsProneFireFixed";
		AISTATE "cultistRecoil", "+5", 5, 0, null, null, null, null, "cultistDodge";
		AISTATE "cultistProneRecoil", "+5", 5, 0, null, null, null, null, "cultistProneDodge";
		AISTATE "cultistTeslaRecoil", "+4", 5, 0, null, null, null, null, "cultistDodge";
		AISTATE "cultistSwimIdle", "+13", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "cultistSwimChase", "+13", 4, 0, null, null, aiMoveForward, cultThinkChase, "none";
		AISTATE "cultistSwimDodge", "+13", 2, 90, null, null, aiMoveDodge, null, "cultistSwimChase";
		AISTATE "cultistSwimGoto", "+13", 2, 600, null, null, aiMoveForward, cultThinkGoto, "cultistSwimIdle";
		AISTATE "cultistSwimSearch", "+13", 3, 1800, null, null, aiMoveForward, cultThinkSearch, "cultistSwimIdle";
		AISTATE "cultistSSwimFire", "+8", 4, 60, ShotSeqCallback, null, null, null, "cultistSwimChase";
		AISTATE "cultistTSwimFire", "+8", 4, 0, TommySeqCallback, null, aiMoveTurn, cultThinkChase, "cultistTSwimFire";
		AISTATE "cultistTsSwimFire", "+8", 4, 0, TeslaSeqCallback, null, aiMoveTurn, cultThinkChase, "cultistTsSwimFire";
		AISTATE "cultistSwimRecoil", "+5", 5, 0, null, null, null, null, "cultistSwimDodge";
		AISTATE "gargoyleFIdle", "+0", 0, 0, null, null, null, gargThinkTarget, "none";
		AISTATE "gargoyleStatueIdle", "+0", 0, 0, null, null, null, null, "none";
		AISTATE "gargoyleFChase", "+0", 4, 0, null, null, gargMoveForward, gargThinkChase, "gargoyleFIdle";
		AISTATE "gargoyleFGoto", "+0", 2, 600, null, null, gargMoveForward, gargThinkGoto, "gargoyleFIdle";
		AISTATE "gargoyleFSlash", "+6", 4, 120, SlashFSeqCallback, null, null, null, "gargoyleFChase";
		AISTATE "gargoyleFThrow", "+6", 4, 120, ThrowFSeqCallback, null, null, null, "gargoyleFChase";
		AISTATE "gargoyleSThrow", "+6", 4, 120, ThrowSSeqCallback, null, gargMoveForward, null, "gargoyleFChase";
		AISTATE "gargoyleSBlast", "+7", 4, 60, BlastSSeqCallback, null, gargMoveSlow, null, "gargoyleFChase";
		AISTATE "gargoyleFRecoil", "+5", 5, 0, null, null, null, null, "gargoyleFChase";
		AISTATE "gargoyleFSearch", "+0", 3, 120, null, null, gargMoveForward, gargThinkSearch, "gargoyleFIdle";
		AISTATE "gargoyleFMorph", "+6", -1, 0, null, null, null, null, "gargoyleFMorph2";
		AISTATE "gargoyleFMorph2", "", -1, 0, null, entryFStatue, NULL, NULL, "gargoyleFIdle";
		AISTATE "gargoyleSMorph", "+6", -1, 0, null, null, null, null, "gargoyleSMorph2";
		AISTATE "gargoyleSMorph2", "", -1, 0, null, entrySStatue, NULL, NULL, "gargoyleStatueIdle";
		AISTATE "gargoyleSwoop", "+0", -1, 120, null, null, gargMoveSwoop, gargThinkChase, "gargoyleFChase";
		AISTATE "gargoyleFly", "+0", 2, 120, null, null, gargMoveFly, gargThinkChase, "gargoyleFChase";
		AISTATE "gargoyleTurn", "+0", 2, 120, null, null, aiMoveTurn, null, "gargoyleFChase";
		AISTATE "gargoyleDodgeUp", "+0", 2, 60, null, null, gargMoveDodgeUp, null, "gargoyleFChase";
		AISTATE "gargoyleFDodgeUpRight", "+0", 2, 90, null, null, gargMoveDodgeUp, null, "gargoyleFChase";
		AISTATE "gargoyleFDodgeUpLeft", "+0", 2, 90, null, null, gargMoveDodgeUp, null, "gargoyleFChase";
		AISTATE "gargoyleDodgeDown", "+0", 2, 120, null, null, gargMoveDodgeDown, null, "gargoyleFChase";
		AISTATE "gargoyleFDodgeDownRight", "+0", 2, 90, null, null, gargMoveDodgeDown, null, "gargoyleFChase";
		AISTATE "gargoyleFDodgeDownLeft", "+0", 2, 90, null, null, gargMoveDodgeDown, null, "gargoyleFChase";
		AISTATE "statueFBreakSEQ", "+5", -1, 0, null, entryFStatue, null, playStatueBreakSnd, "gargoyleFMorph2";
		AISTATE "statueSBreakSEQ", "+5", -1, 0, null, entrySStatue, null, playStatueBreakSnd, "gargoyleSMorph2";
		AISTATE "ghostIdle", "+0", 0, 0, null, null, null, ghostThinkTarget, "none";
		AISTATE "ghostChase", "+0", 4, 0, null, null, ghostMoveForward, ghostThinkChase, "ghostIdle";
		AISTATE "ghostGoto", "+0", 2, 600, null, null, ghostMoveForward, ghostThinkGoto, "ghostIdle";
		AISTATE "ghostSlash", "+6", 4, 120, ghostSlashSeqCallback, null, null, null, "ghostChase";
		AISTATE "ghostThrow", "+6", 4, 120, ghostThrowSeqCallback, null, null, null, "ghostChase";
		AISTATE "ghostBlast", "+6", 4, 120, ghostBlastSeqCallback, null, ghostMoveSlow, null, "ghostChase";
		AISTATE "ghostRecoil", "+5", 5, 0, null, null, null, null, "ghostChase";
		AISTATE "ghostTeslaRecoil", "+4", 5, 0, null, null, null, null, "ghostChase";
		AISTATE "ghostSearch", "+0", 3, 120, null, null, ghostMoveForward, ghostThinkSearch, "ghostIdle";
		AISTATE "ghostSwoop", "+0", -1, 120, null, null, ghostMoveSwoop, ghostThinkChase, "ghostChase";
		AISTATE "ghostFly", "+0", 2, 0, null, null, ghostMoveFly, ghostThinkChase, "ghostChase";
		AISTATE "ghostTurn", "+0", 2, 120, null, null, aiMoveTurn, null, "ghostChase";
		AISTATE "ghostDodgeUp", "+0", 2, 60, null, null, ghostMoveDodgeUp, null, "ghostChase";
		AISTATE "ghostDodgeUpRight", "+0", 2, 90, null, null, ghostMoveDodgeUp, null, "ghostChase";
		AISTATE "ghostDodgeUpLeft", "+0", 2, 90, null, null, ghostMoveDodgeUp, null, "ghostChase";
		AISTATE "ghostDodgeDown", "+0", 2, 120, null, null, ghostMoveDodgeDown, null, "ghostChase";
		AISTATE "ghostDodgeDownRight", "+0", 2, 90, null, null, ghostMoveDodgeDown, null, "ghostChase";
		AISTATE "ghostDodgeDownLeft", "+0", 2, 90, null, null, ghostMoveDodgeDown, null, "ghostChase";
		AISTATE "gillBeastIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "gillBeastChase", "+9", 4, 0, null, null, aiMoveForward, gillThinkChase, "none";
		AISTATE "gillBeastDodge", "+9", 2, 90, null, null, aiMoveDodge, null, "gillBeastChase";
		AISTATE "gillBeastGoto", "+9", 2, 600, null, null, aiMoveForward, gillThinkGoto, "gillBeastIdle";
		AISTATE "gillBeastBite", "+6", 4, 120, GillBiteSeqCallback, null, null, null, "gillBeastChase";
		AISTATE "gillBeastSearch", "+9", 2, 120, null, null, aiMoveForward, gillThinkSearch, "gillBeastIdle";
		AISTATE "gillBeastRecoil", "+5", 5, 0, null, null, null, null, "gillBeastDodge";
		AISTATE "gillBeastSwimIdle", "+10", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "gillBeastSwimChase", "+10", 4, 0, null, null, gillMoveSwimChase, gillThinkSwimChase, "none";
		AISTATE "gillBeastSwimDodge", "+10", 2, 90, null, null, aiMoveDodge, null, "gillBeastSwimChase";
		AISTATE "gillBeastSwimGoto", "+10", 2, 600, null, null, aiMoveForward, gillThinkSwimGoto, "gillBeastSwimIdle";
		AISTATE "gillBeastSwimSearch", "+10", 3, 120, null, null, aiMoveForward, gillThinkSearch, "gillBeastSwimIdle";
		AISTATE "gillBeastSwimBite", "+7", 4, 0, GillBiteSeqCallback, null, null, gillThinkSwimChase, "gillBeastSwimChase";
		AISTATE "gillBeastSwimRecoil", "+5", 5, 0, null, null, null, null, "gillBeastSwimDodge";
		AISTATE "gillBeastSwimUnused", "+10", -1, 120, null, null, gillMoveSwimUnused, gillThinkSwimChase, "gillBeastSwimChase";
		AISTATE "gillBeastSwimMoveIn", "+10", -1, 0, null, null, gillSwimMoveIn, gillThinkSwimChase, "gillBeastSwimChase";
		AISTATE "gillBeastSwimTurn", "+10", -1, 120, null, null, null, aiMoveTurn, "gillBeastSwimChase";
		AISTATE "handIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "handChoke", "+0", -1, 0, null, null, null, null, "none";
		AISTATE "handSearch", "+6", 2, 600, null, null, aiMoveForward, handThinkSearch, "handIdle";
		AISTATE "handChase", "+6", 4, 0, null, null, aiMoveForward, handThinkChase, "none";
		AISTATE "handRecoil", "+5", 5, 0, null, null, null, null, "handSearch";
		AISTATE "handGoto", "+6", 2, 1800, null, null, aiMoveForward, handThinkGoto, "handIdle";
		AISTATE "handJump", "+7", 4, 120, HandJumpSeqCallback, null, null, null, "handChase";
		AISTATE "houndIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "houndSearch", "+8", 2, 1800, null, null, aiMoveForward, houndThinkSearch, "houndIdle";
		AISTATE "houndChase", "+8", 4, 0, null, null, aiMoveForward, houndThinkChase, "none";
		AISTATE "houndRecoil", "+5", 5, 0, null, null, null, null, "houndSearch";
		AISTATE "houndTeslaRecoil", "+4", 5, 0, null, null, null, null, "houndSearch";
		AISTATE "houndGoto", "+8", 2, 600, null, null, aiMoveForward, houndThinkGoto, "houndIdle";
		AISTATE "houndBite", "+6", 4, 60, houndBiteSeqCallback, null, null, null, "houndChase";
		AISTATE "houndBurn", "+7", 4, 60, houndBurnSeqCallback, null, null, null, "houndChase";
		AISTATE "innocentIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "innocentSearch", "+6", 3, 1800, null, null, aiMoveForward, innocThinkSearch, "innocentIdle";
		AISTATE "innocentChase", "+6", 4, 0, null, null, aiMoveForward, innocThinkChase, "none";
		AISTATE "innocentRecoil", "+5", 5, 0, null, null, null, null, "innocentChase";
		AISTATE "innocentTeslaRecoil", "+4", 5, 0, null, null, null, null, "innocentChase";
		AISTATE "innocentGoto", "+6", 2, 600, null, null, aiMoveForward, innocThinkGoto, "innocentIdle";
		AISTATE "podIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "podMove", "+7", 2, 3600, null, null, aiMoveTurn, aiPodMove, "podSearch";
		AISTATE "podSearch", "+0", 3, 3600, null, null, aiMoveTurn, aiPodSearch, "podSearch";
		AISTATE "podStartChase", "+8", 4, 600, podAttack, null, null, null, "podChase";
		AISTATE "podRecoil", "+5", 5, 0, null, null, null, null, "podChase";
		AISTATE "podChase", "+6", 4, 0, null, null, aiMoveTurn, aiPodChase, "none";
		AISTATE "tentacleIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "tentaclePlaySound1", "+7", -1, 0, podPlaySound1, null, null, null, "tentaclePlaySound1a";
		AISTATE "tentaclePlaySound1a", "", -1, 0, null, null, null, null, "tentacleChase";
		AISTATE "tentaclePlaySound2", "+8", -1, 0, podPlaySound2, null, null, null, "tentaclePlaySound2a";
		AISTATE "tentaclePlaySound2a", "", -1, 0, null, null, null, null, "tentacleChase";
		AISTATE "tentacleMove", "+8", -1, 3600, null, null, aiMoveTurn, aiPodMove, "tentacleSearch";
		AISTATE "tentacleSearch", "+0", -1, 3600, null, null, aiMoveTurn, aiPodSearch, "none";
		AISTATE "tentacleStartChase", "+6", -1, 120, podExplode, null, null, null, "tentacleChase";
		AISTATE "tentacleRecoil", "+5", 5, 0, null, null, null, null, "tentacleChase";
		AISTATE "tentacleChase", "+6", 4, 0, null, null, aiMoveTurn, aiPodChase, "none";
		AISTATE "ratIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "ratSearch", "+7", 3, 1800, null, null, aiMoveForward, ratThinkSearch, "ratIdle";
		AISTATE "ratChase", "+7", 4, 0, null, null, aiMoveForward, ratThinkChase, "none";
		AISTATE "ratDodge", "+7", 2, 0, null, null, null, null, "ratChase";
		AISTATE "ratRecoil", "+7", 5, 0, null, null, null, null, "ratDodge";
		AISTATE "ratGoto", "+7", 2, 600, null, null, aiMoveForward, ratThinkGoto, "ratIdle";
		AISTATE "ratBite", "+6", 4, 120, ratBiteSeqCallback, null, null, null, "ratChase";
		AISTATE "spidIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "spidChase", "+7", 4, 0, null, null, aiMoveForward, spidThinkChase, "none";
		AISTATE "spidDodge", "+7", 2, 90, null, null, aiMoveDodge, null, "spidChase";
		AISTATE "spidGoto", "+7", 2, 600, null, null, aiMoveForward, spidThinkGoto, "spidIdle";
		AISTATE "spidSearch", "+7", 3, 1800, null, null, aiMoveForward, spidThinkSearch, "spidIdle";
		AISTATE "spidBite", "+6", 4, 60, SpidBiteSeqCallback, null, null, null, "spidChase";
		AISTATE "spidJump", "+8", 4, 60, SpidJumpSeqCallback, null, aiMoveForward, null, "spidChase";
		AISTATE "spidBirth", "+0", -1, 60, SpidBirthSeqCallback, null, null, null, "spidIdle";
		AISTATE "tchernobogIdle", "+0", 0, 0, null, null, null, tchernobogThinkTarget, "none";
		AISTATE "tchernobogSearch", "+8", 3, 1800, null, null, aiMoveForward, tchernobogThinkSearch, "tchernobogIdle";
		AISTATE "tchernobogChase", "+8", 4, 0, null, null, aiMoveForward, tchernobogThinkChase, "none";
		AISTATE "tchernobogRecoil", "+5", 5, 0, null, null, null, null, "tchernobogSearch";
		AISTATE "tchernobogGoto", "+8", 2, 600, null, null, aiMoveForward, tchernobogThinkGoto, "tchernobogIdle";
		AISTATE "tchernobogBurn1", "+6", 2, 60, tchernobogBurnSeqCallback, null, null, null, "tchernobogChase";
		AISTATE "tchernobogBurn2", "+6", 4, 60, tchernobogBurnSeqCallback2, null, null, null, "tchernobogChase";
		AISTATE "tchernobogFireAtk", "+7", 4, 60, tchernobogFire, null, null, null, "tchernobogChase";
		AISTATE "tchernobogTurn", "+8", 4, 60, null, null, aiMoveTurn, null, "tchernobogChase";
		AISTATE "zombieAIdle", "+0", 0, 0, null, entryAIdle, null, aiThinkTarget, "none";
		AISTATE "zombieAChase", "+8", 4, 0, null, null, aiMoveForward, zombaThinkChase, "none";
		AISTATE "zombieAPonder", "+0", -1, 0, null, null, aiMoveTurn, zombaThinkPonder, "none";
		AISTATE "zombieAGoto", "+8", 2, 1800, null, null, aiMoveForward, zombaThinkGoto, "zombieAIdle";
		AISTATE "zombieAHack", "+6", 4, 80, HackSeqCallback, null, null, null, "zombieAPonder";
		AISTATE "zombieASearch", "+8", 3, 1800, null, null, aiMoveForward, zombaThinkSearch, "zombieAIdle";
		AISTATE "zombieARecoil", "+5", 5, 0, null, null, null, null, "zombieAPonder";
		AISTATE "zombieATeslaRecoil", "+4", 5, 0, null, null, null, null, "zombieAPonder";
		AISTATE "zombieARecoil2", "+1", 5, 360, null, null, null, null, "zombieAStand";
		AISTATE "zombieAStand", "+11", 2, 0, StandSeqCallback, null, null, null, "zombieAPonder";
		AISTATE "zombieEIdle", "+12", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "zombieEUp2", "+0", 2, 1, null, entryEZombie, null, null, "zombieASearch";
		AISTATE "zombieEUp", "+9", 2, 180, null, entryEStand, null, null, "zombieEUp2";
		AISTATE "zombie2Idle", "+0", 0, 0, null, entryAIdle, null, myThinkTarget, "none";
		AISTATE "zombie2Search", "+8", 3, 1800, null, null, null, myThinkSearch, "zombie2Idle";
		AISTATE "zombieSIdle", "+10", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "zombieEStand", "+11", -1, 0, StandSeqCallback, entryEZombie, null, null, "zombieAPonder";
		AISTATE "zombieFIdle", "+0", 0, 0, null, null, null, aiThinkTarget, "none";
		AISTATE "zombieFChase", "+8", 4, 0, null, null, aiMoveForward, zombfThinkChase, "none";
		AISTATE "zombieFGoto", "+8", 2, 600, null, null, aiMoveForward, zombfThinkGoto, "zombieFIdle";
		AISTATE "zombieFDodge", "+8", 2, 0, null, null, aiMoveDodge, zombfThinkChase, "zombieFChase";
		AISTATE "zombieFHack", "+6", 4, 120, zombfHackSeqCallback, null, null, null, "zombieFChase";
		AISTATE "zombieFPuke", "+9", 4, 120, PukeSeqCallback, null, null, null, "zombieFChase";
		AISTATE "zombieFThrow", "+6", 4, 120, ThrowSeqCallback, null, null, null, "zombieFChase";
		AISTATE "zombieFSearch", "+8", 3, 1800, null, null, aiMoveForward, zombfThinkSearch, "zombieFIdle";
		AISTATE "zombieFRecoil", "+5", 5, 0, null, null, null, null, "zombieFChase";
		AISTATE "zombieFTeslaRecoil", "+4", 5, 0, null, null, null, null, "zombieFChase";
		preloadseq 0, 1, 2, 5;
	}
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
	default
	{
		seqstartid 12032;
		health 100;
		mass 70;
		clipdist 12;
		aimheight 16;
		heardist 128;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 11.25;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;
		+BloodDudeBase.floorhitdamage;
	}

	override int getRespawnTime()
	{
		return -1;	// no respawn for players.
	}
}

class BloodDudeCultistTommy : BloodDudeBase
{
	default
	{
		seqstartid 4096;
		health 40;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 8;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 96, 256, 256, 256, 192;

		explodesound 717;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9, 13, 14, 15;
	}
}

class BloodDudeCultistShotgun : BloodDudeBase
{
	default
	{
		seqstartid 11520;
		health 40;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 5;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.533325195;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 128, 256, 256, 256, 192;

		explodesound 717;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9, 13, 14, 15;
	}
}

class BloodDudeZombieAxeNormal : BloodDudeBase
{
	default
	{
		seqstartid 4352;
		health 60;
		mass 70;
		clipdist 12;
		eyeheight 46;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 112, 256, 256, 256, 160;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 11, 13, 14;
	}
}

class BloodDudeZombieButcher : BloodDudeBase
{
	default
	{
		seqstartid 4608;
		health 80;
		mass 200;
		clipdist 12;
		eyeheight 128;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.355545044;
		sidespeed 0.355545044;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 32, 128, 256, 64, 128;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9, 10, 11;
	}
}

class BloodDudeZombieAxeBuried : BloodDudeBase
{
	default
	{
		seqstartid 4352;
		health 60;
		mass 70;
		clipdist 12;
		eyeheight 46;
		aimheight 20;
		heardist 320;
		seedist 0;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 112, 256, 256, 256, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 12, 9, 10;
	}
}

class BloodDudeGargoyleFlesh : BloodDudeBase
{
	default
	{
		seqstartid 4864;
		health 110;
		mass 120;
		clipdist 16;
		eyeheight 13;
		aimheight 5;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 25;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.355545044;
		turnrange 67.5;
		gibtype 30, -1, -1;
		dmgcontrol 0, 128, 48, 208, 256, 256, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9;
	}
}

class BloodDudeGargoyleStone : BloodDudeBase
{
	default
	{
		seqstartid 5120;
		health 200;
		mass 200;
		clipdist 21;
		eyeheight 13;
		aimheight 5;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 20;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.355545044;
		turnrange 45;
		gibtype 19, -1, -1;
		dmgcontrol 0, 0, 10, 10, 0, 128, 64;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9;
	}
}

class BloodDudeGargoyleStatueFlesh : BloodDudeBase
{
	default
	{
		seqstartid 11008;
		health 100;
		mass 200;
		clipdist 16;
		eyeheight 13;
		aimheight 5;
		heardist 128;
		seedist 320;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 0;
		gibtype -1, -1, -1;
		preloadseq 6, 7, 8, 9;
	}
}

class BloodDudeGargoyleStatueStone : BloodDudeBase
{
	default
	{
		seqstartid 11264;
		health 100;
		mass 200;
		clipdist 16;
		eyeheight 13;
		aimheight 5;
		heardist 128;
		seedist 320;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 0;
		gibtype -1, -1, -1;
		preloadseq 6, 7, 8, 9;
	}
}

class BloodDudePhantasm : BloodDudeBase
{
	default
	{
		seqstartid 5376;
		health 100;
		mass 70;
		clipdist 16;
		eyeheight 25;
		aimheight 15;
		heardist 640;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype -1, -1, -1;
		dmgcontrol 0, 0, 48, 0, 0, 16, 0;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeHellHound : BloodDudeBase
{
	default
	{
		seqstartid 5632;
		health 70;
		mass 120;
		clipdist 20;
		eyeheight 6;
		heardist 640;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 20;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 1.777771;
		sidespeed 1.24443054;
		backspeed 1.06666565;
		turnrange 67.5;
		gibtype 29, -1, -1;
		dmgcontrol 48, 0, 48, 48, 256, 128, 192;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeHand : BloodDudeBase
{
	default
	{
		seqstartid 5888;
		health 10;
		mass 70;
		clipdist 8;
		heardist 320;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 64, 256, 256, 256, 0, 64, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudeSpiderBrown : BloodDudeBase
{
	default
	{
		seqstartid 6144;
		health 10;
		mass 5;
		clipdist 8;
		eyeheight -5;
		aimheight -5;
		heardist 320;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 64, 256, 256, 96, 256, 64, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeSpiderRed : BloodDudeBase
{
	default
	{
		seqstartid 6400;
		health 25;
		mass 10;
		clipdist 8;
		eyeheight -5;
		aimheight -5;
		heardist 320;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 64, 128, 256, 96, 256, 64, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeSpiderBlack : BloodDudeBase
{
	default
	{
		seqstartid 6656;
		health 75;
		mass 20;
		clipdist 8;
		eyeheight -5;
		aimheight -5;
		heardist 320;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 128, 256, 256, 96, 256, 64, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeSpiderMother : BloodDudeBase
{
	default
	{
		seqstartid 6912;
		health 100;
		mass 40;
		clipdist 8;
		eyeheight -5;
		aimheight -5;
		heardist 320;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 32, 16, 16, 16, 32, 32, 32;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeGillBeast : BloodDudeBase
{
	default
	{
		seqstartid 7168;
		health 50;
		mass 200;
		clipdist 16;
		eyeheight 37;
		aimheight 20;
		heardist 320;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 48, 80, 64, 128, 0, 128, 48;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8, 9, 10, 11;
	}
}

class BloodDudeBoneEel : BloodDudeBase
{
	default
	{
		seqstartid 7424;
		health 25;
		mass 30;
		clipdist 8;
		eyeheight 4;
		heardist 320;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.533325195;
		sidespeed 0.355545044;
		backspeed 0.355545044;
		turnrange 22.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 0, 256, 192;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudeBat : BloodDudeBase
{
	default
	{
		seqstartid 7680;
		health 10;
		mass 5;
		clipdist 8;
		eyeheight 2;
		heardist 640;
		seedist 1600;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.355545044;
		sidespeed 0.355545044;
		backspeed 0.21333313;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 64, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudeRat : BloodDudeBase
{
	default
	{
		seqstartid 7936;
		health 10;
		mass 5;
		clipdist 8;
		eyeheight 3;
		heardist 800;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 128, 256;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudePodGreen : BloodDudeBase
{
	default
	{
		seqstartid 8192;
		health 50;
		mass 65535;
		clipdist 16;
		eyeheight 40;
		heardist 128;
		seedist 704;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 160, 160, 128, 160, 0, 0, 256;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudeTentacleGreen : BloodDudeBase
{
	default
	{
		seqstartid 8448;
		health 10;
		mass 65535;
		clipdist 8;
		heardist 128;
		seedist 320;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 80, 0, 0, 256;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudePodFire : BloodDudeBase
{
	default
	{
		seqstartid 8704;
		health 100;
		mass 65535;
		clipdist 16;
		eyeheight 40;
		heardist 128;
		seedist 960;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 96, 0, 128, 64, 256, 64, 160;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudeTentacleFire : BloodDudeBase
{
	default
	{
		seqstartid 8960;
		health 20;
		mass 65535;
		clipdist 8;
		heardist 128;
		seedist 320;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 128, 0, 128, 128, 0, 0, 128;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudePodMother : BloodDudeBase
{
	default
	{
		seqstartid 9216;
		health 200;
		mass 65535;
		clipdist 16;
		eyeheight 40;
		heardist 128;
		seedist 3200;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 0;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudeTentacleMother : BloodDudeBase
{
	default
	{
		seqstartid 9472;
		health 50;
		mass 65535;
		clipdist 8;
		heardist 128;
		seedist 3200;
		periphery 180;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 0;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 128, 256, 128, 128, 256;
		+BloodDudeBase.floorhitdamage;
	}
}

class BloodDudeCerberusTwoHead : BloodDudeBase
{
	default
	{
		seqstartid 9728;
		health 200;
		mass 1000;
		clipdist 16;
		eyeheight 29;
		aimheight 10;
		heardist 2560;
		seedist 6400;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 1.06666565;
		sidespeed 0.888885498;
		backspeed 0.711105347;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 16, 0, 16, 16, 0, 96, 48;
		deathMorphType "BloodDudeCerberusOneHead";
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudeCerberusOneHead : BloodDudeBase
{
	default
	{
		seqstartid 9984;
		health 100;
		mass 1000;
		clipdist 16;
		eyeheight 29;
		aimheight 10;
		heardist 1280;
		seedist 3200;
		periphery 119.882813;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.533325195;
		backspeed 0.391098022;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 16, 0, 16, 16, 0, 96, 48;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7;
	}
}

class BloodDudeTchernobog : BloodDudeBase
{
	default
	{
		seqstartid 10240;
		health 32;
		mass 1500;
		clipdist 32;
		heardist 1600;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.888885498;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 3, 1, 4, 4, 0, 4, 3;
		+BloodDudeBase.floorhitdamage;
		preloadseq 6, 7, 8;
	}
}

class BloodDudeCultistTommyProne : BloodDudeBase
{
	default
	{
		seqstartid 4096;
		health 25;
		mass 20;
		clipdist 8;
		heardist 128;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 96, 256, 256, 256, 192;

		explodesound 717;
	}
}

class BloodDudePlayer1 : BloodPlayerBase
{
}

class BloodDudePlayer2 : BloodPlayerBase
{
}

class BloodDudePlayer3 : BloodPlayerBase
{
}

class BloodDudePlayer4 : BloodPlayerBase
{
}

class BloodDudePlayer5 : BloodPlayerBase
{
}

class BloodDudePlayer6 : BloodPlayerBase
{
}

class BloodDudePlayer7 : BloodPlayerBase
{
}

class BloodDudePlayer8 : BloodPlayerBase
{
}

class BloodDudeBurningInnocent : BloodDudeBase
{
	default
	{
		seqstartid 12544;
		health 25;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 100;
		hinderdamage 100;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 28.125;
		gibtype 7, 5, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "none";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		explodesound 717;
		+BloodDudeBase.burning;
	}

	states
	{
	}
}

class BloodDudeBurningCultist : BloodDudeBase
{
	default
	{
		seqstartid 4096;
		health 30;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 100;
		hinderdamage 100;
		alertchance 32768;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 28.125;
		gibtype 7, 5, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		//AISTATE "BurnIdle", "+3", 0, 0, null, null, null, aiThinkTarget, "none"; //unused
		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "BurnSearch";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		explodesound 717;
		+BloodDudeBase.burning;
	}
}

class BloodDudeBurningZombieAxe : BloodDudeBase
{
	default
	{
		seqstartid 4352;
		health 12;
		mass 70;
		clipdist 12;
		eyeheight 46;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 28.125;
		gibtype 7, 5, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "none";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		+BloodDudeBase.burning;
	}
}

class BloodDudeBurningZombieButcher : BloodDudeBase
{
	default
	{
		seqstartid 4352;
		health 25;
		mass 120;
		clipdist 12;
		eyeheight 44;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.604431152;
		sidespeed 0.42666626;
		backspeed 0.21333313;
		turnrange 17.578125;
		gibtype 7, 5, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "none";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		+BloodDudeBase.burning;
	}
}

class BloodDudeCultistReserved : BloodDudeBase // this is never checked as a cultist in the code.
{
	default
	{
		seqstartid 4096;
		health 100;
		mass 70;
		clipdist 16;
		eyeheight 38;
		aimheight 20;
		heardist 128;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 11.25;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;
	}
}

class BloodDudeZombieAxeLaying : BloodDudeBase
{
	default
	{
		seqstartid 4352;
		health 60;
		mass 70;
		clipdist 12;
		eyeheight 46;
		aimheight 20;
		heardist 320;
		seedist 0;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 15;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 112, 256, 256, 256, 256;
		preloadseq 10;
	}
}

class BloodDudeInnocent : BloodDudeBase
{
	default
	{
		seqstartid 12544;
		health 50;
		mass 70;
		clipdist 12;
		eyeheight 46;
		aimheight 20;
		heardist 160;
		seedist 0;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 8;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 15, -1, -1;
		dmgcontrol 288, 288, 288, 288, 288, 288, 288;

		explodesound 717;
	}
}

class BloodDudeCultistShotgunProne : BloodDudeBase
{
	default
	{
		seqstartid 11520;
		health 25;
		mass 70;
		clipdist 8;
		eyeheight -5;
		heardist 128;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0;
		sidespeed 0;
		backspeed 0;
		turnrange 11.25;
		gibtype 7, 5, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		explodesound 717;
	}
}

class BloodDudeCultistTesla : BloodDudeBase
{
	default
	{
		seqstartid 12800;
		health 40;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 8;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 256, 256, 96, 160, 256, 256, 12;

		explodesound 717;
		preloadseq 6, 7, 8, 9, 13, 14, 15;
	}
}

class BloodDudeCultistTNT : BloodDudeBase
{
	default
	{
		seqstartid 13056;
		health 40;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 8;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 256, 160, 96, 64, 256, 256, 256;

		explodesound 717;
		preloadseq 6, 7, 8, 9, 13, 14, 15;
	}
}

class BloodDudeCultistBeast : BloodDudeBase
{
	default
	{
		seqstartid 13312;
		health 40;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 640;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 12;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 0.711105347;
		sidespeed 0.533325195;
		backspeed 0.21333313;
		turnrange 45;
		gibtype 15, -1, -1;
		dmgcontrol 128, 128, 16, 16, 0, 64, 48;

		explodesound 717;
		preloadseq 6, 7;
	}
}

class BloodDudeTinyCaleb : BloodDudeBase
{
	default
	{
		seqstartid 13568;
		health 10;
		mass 5;
		clipdist 8;
		eyeheight 3;
		heardist 800;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 160, 160, 160, 160, 256, 128, 288;

		explodesound 717;
	}
}

class BloodDudeBeast : BloodDudeBase
{
	default
	{
		seqstartid 10752;
		health 120;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 800;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 1.777771;
		sidespeed 1.24443054;
		backspeed 1.06666565;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 5, 5, 15, 8, 0, 15, 15;
	}
}

class BloodDudeBurningTinyCaleb : BloodDudeBase
{
	default
	{
		seqstartid 13568;
		health 10;
		mass 5;
		clipdist 8;
		eyeheight 3;
		heardist 800;
		seedist 3200;
		periphery 90;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		frontspeed 0.888885498;
		sidespeed 0.711105347;
		backspeed 0.533325195;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "BurnSearch";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		explodesound 717;
		+BloodDudeBase.burning;
	}
}

class BloodDudeBurningBeast : BloodDudeBase
{
	default
	{
		seqstartid 10752;
		health 25;
		mass 70;
		clipdist 12;
		eyeheight 41;
		aimheight 20;
		heardist 800;
		seedist 3200;
		periphery 59.9414063;
		meleedist 0;
		fleehealth 10;
		hinderdamage 10;
		changetargetchance 256;
		changetargetkinchance 16;
		alertchance 32768;
		lockout 1;
		frontspeed 1.777771;
		sidespeed 1.24443054;
		backspeed 1.06666565;
		turnrange 67.5;
		gibtype 7, -1, -1;
		dmgcontrol 256, 256, 256, 256, 256, 256, 256;

		AISTATE "BurnChase", "+3", 4, 0, null, null, aiMoveForward, burnThinkChase, "none";
		AISTATE "BurnGoto", "+3", 2, 3600, null, null, aiMoveForward, burnThinkGoto, "BurnSearch";
		AISTATE "BurnSearch", "+3", 3, 3600, null, null, aiMoveForward, burnThinkSearch, "BurnSearch";
		AISTATE "BurnAttack", "+3", 4, 120, BurnSeqCallback, null, null, null, "BurnChase";

		+BloodDudeBase.burning;
	}
}

