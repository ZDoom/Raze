#include "ns.h"

BEGIN_WH_NS


extern int S_THUNDER1; //0;
extern int S_THUNDER2; //1;
extern int S_THUNDER3; //2;
extern int S_THUNDER4; //3;
extern int S_WINDLOOP1; //4;
extern int S_WAVELOOP1; //5;
extern int S_LAVALOOP1; //6;
extern int S_FIRELOOP1; //7;
extern int S_STONELOOP1; //8;
extern int S_BATSLOOP; //9;
extern int S_PLRWEAPON0; //10;
extern int S_PLRWEAPON1; //11;
extern int S_PLRWEAPON2; //12;
extern int S_PLRWEAPON3; //13;
extern int S_PLRWEAPON4; //14;
extern int S_GOBLIN1; //15;
extern int S_GOBLIN2; //16;
extern int S_GOBLIN3; //17;
extern int S_GOBPAIN1; //18;
extern int S_GOBPAIN2; //19;
extern int S_GOBDIE1; //20;
extern int S_GOBDIE2; //21;
extern int S_GOBDIE3; //22;
extern int S_KSNARL1; //23;
extern int S_KSNARL2; //24;
extern int S_KSNARL3; //25;
extern int S_KSNARL4; //26;
extern int S_KPAIN1; //27;
extern int S_KPAIN2; //28;
extern int S_KDIE1; //29;
extern int S_KDIE2; //30;
extern int S_DEMON1; //31;
extern int S_DEMON2; //32;
extern int S_DEMON3; //33;
extern int S_DEMON4; //34;
extern int S_DEMON5; //35;
extern int S_DEMONDIE1; //36;
extern int S_DEMONDIE2; //37;
extern int S_MSNARL1; //38;
extern int S_MSNARL2; //39;
extern int S_MSNARL3; //40;
extern int S_MSNARL4; //41;
extern int S_MPAIN1; //42;
extern int S_MDEATH1; //43;
extern int S_DRAGON1; //44;
extern int S_DRAGON2; //45;
extern int S_DRAGON3; //46;
extern int S_RIP1; //47;
extern int S_RIP2; //48;
extern int S_RIP3; //49;
extern int S_SKELHIT1; //50;
extern int S_SKELHIT2; //51;
extern int S_SKELETONDIE; //52;
extern int S_GUARDIAN1; //53;
extern int S_GUARDIAN2; //54;
extern int S_GUARDIANPAIN1; //55;
extern int S_GUARDIANPAIN2; //56;
extern int S_GUARDIANDIE; //57;
extern int S_WISP; //58;
extern int S_WISP2; //59;
extern int S_SPLASH1; //60;
extern int S_SPLASH2; //61;
extern int S_SPLASH3; //62;
extern int S_SPLASH4; //63;
extern int S_SPLASH5; //64;
extern int S_SPLASH6; //65;
extern int S_WILLOWDIE; //66;
extern int S_FATWITCHDIE; //67;
extern int S_JUDY1; //68;
extern int S_JUDY2; //69;
extern int S_JUDY3; //70;
extern int S_JUDY4; //71;
extern int S_JUDYDIE; //72;
extern int S_SKULLWITCH1; //73;
extern int S_SKULLWITCH2; //74;
extern int S_SKULLWITCH3; //75;
extern int S_SKULLWITCHDIE; //76;
extern int S_GRONDEATHA; //77;
extern int S_GRONDEATHB; //78;
extern int S_GRONDEATHC; //79;
extern int S_CHAIN1; //80;
extern int S_FLAME1; //81;
extern int S_GRONPAINA; //82;
extern int S_GRONPAINB; //83;
extern int S_GRONPAINC; //84;
extern int S_CLUNK; //85;
extern int S_DEMONTHROW; //86;
extern int S_WITCHTHROW; //87;
extern int S_DOOR1; //88;
extern int S_DOOR2; //89;
extern int S_DOOR3; //90;
extern int S_CREAKDOOR1; //91;
extern int S_CREAKDOOR2; //92;
extern int S_CREAKDOOR3; //93;
extern int S_STING1; //94;
extern int S_STING2; //95;
extern int S_POTION1; //96;
extern int S_GENTHROW; //97;
extern int S_GENSWING; //98;
extern int S_ARROWHIT; //99;
extern int S_WALLHIT1; //100;
extern int S_GONG; //101;
extern int S_SPELL1; //102;
extern int S_FREEZE; //103;
extern int S_FREEZEDIE; //104;
extern int S_TRAP1; //105;
extern int S_TRAP2; //106;
extern int S_RATS1; //107;
extern int S_RATS2; //108;
extern int S_WINDLOOP2; //109;
extern int S_BREATH1; //110;
extern int S_BREATH2; //111;
extern int S_PUSH1; //112;
extern int S_PUSH2; //113;
extern int S_PLRPAIN1; //114;
extern int S_PLRPAIN2; //115;
extern int S_GORE1; //116;
extern int S_GORE2; //117;
extern int S_GORE1A; //118;
extern int S_GORE1B; //119;
extern int S_DEADSTEP; //120;
extern int S_HEARTBEAT; //121;
extern int S_SOFTCHAINWALK; //122;
extern int S_SOFTCREAKWALK; //123;
extern int S_LOUDCHAINWALK; //124;
extern int S_GRATEWALK; //125;
extern int S_SCARYDUDE; //126;
extern int S_WATERY; //127;
extern int S_GLASSBREAK1; //128;
extern int S_GLASSBREAK2; //129;
extern int S_GLASSBREAK3; //130;
extern int S_TREASURE1; //131;
extern int S_SWORD1; //132;
extern int S_SWORD2; //133;
extern int S_SWORDCLINK1; //134;
extern int S_SWORDCLINK2; //135;
extern int S_SWORDCLINK3; //136;
extern int S_SWORDCLINK4; //137;
extern int S_SOCK1; //138;
extern int S_SOCK2; //139;
extern int S_SOCK3; //140;
extern int S_SOCK4; //141;
extern int S_KOBOLDHIT; //142;
extern int S_SPIDERBITE; //143;
extern int S_FIREBALL; //144;
extern int S_WOOD1; //145;
extern int S_CHAINDOOR1; //146;
extern int S_PULLCHAIN1; //147;
extern int S_PICKUPAXE; //148;
extern int S_EXPLODE; //149;
extern int S_SKELSEE; //150;
extern int S_BARRELBREAK; //151;
extern int S_WARP; //152;
extern int S_PICKUPFLAG; //155;
extern int S_DROPFLAG; //156;
extern int S_LAUGH; //157;
extern int S_DRINK; //158;
extern int S_PAGE; //159;
extern int S_BOTTLES; //160;
extern int S_CRACKING; //161;
extern int S_PLRDIE1; //162;
extern int S_FATLAUGH; //163;


enum WHSounds {

	S_SPELL2 = 153,
	S_THROWPIKE = 154,

	// WH2 Sounds
	S_AGM_ATTACK = 129,
	S_AGM_PAIN1 = 130,
	S_AGM_PAIN2 = 131,
	S_AGM_PAIN3 = 132,
	S_AGM_DIE1 = 133,
	S_AGM_DIE2 = 134,
	S_AGM_DIE3 = 135,
	S_FIRESWING = 145,
	S_FIREWEAPONLOOP = 146,
	S_ENERGYWEAPONLOOP = 147,
	S_ENERGYSWING = 148,
	S_BIGGLASSBREAK1 = 149,
	S_IMPGROWL1 = 152,
	S_IMPGROWL2 = 153,
	S_IMPGROWL3 = 154,
	S_IMPDIE1 = 155,
	S_IMPDIE2 = 156,
	S_SWINGDOOR = 157,
	S_NUKESPELL = 158,
	S_DOORSPELL = 159,
	S_FIRESPELL = 160,
	S_GENERALMAGIC1 = 161,
	S_GENERALMAGIC2 = 162,
	S_GENERALMAGIC3 = 163,
	S_GENERALMAGIC4 = 164,
};

END_WH_NS
