-- Use this file like
--  require("lpeg")
--  con = require("con_lang")
--
-- Contains:
--   * con.labels
--   * con.keyword

local lpeg = lpeg


module(...)


MAXVOLUMES = 7
MAXLEVELS = 64

MAXSKILLS = 7

MAXSOUNDS = 4096


-- KEEPINSYNC gamedef.h
labels =
{
    {
        STR_MAPNAME = 0,
        STR_MAPFILENAME = 1,
        STR_PLAYERNAME = 2,
        STR_VERSION = 3,
        STR_GAMETYPE = 4,
        STR_VOLUMENAME = 5,
    },

    {
        PROJ_WORKSLIKE = 0,
        PROJ_SPAWNS = 1,
        PROJ_SXREPEAT = 2,
        PROJ_SYREPEAT = 3,
        PROJ_SOUND = 4,
        PROJ_ISOUND = 5,
        PROJ_VEL = 6,
        PROJ_EXTRA = 7,
        PROJ_DECAL = 8,
        PROJ_TRAIL = 9,
        PROJ_TXREPEAT = 10,
        PROJ_TYREPEAT = 11,
        PROJ_TOFFSET = 12,
        PROJ_TNUM = 13,
        PROJ_DROP = 14,
        PROJ_CSTAT = 15,
        PROJ_CLIPDIST = 16,
        PROJ_SHADE = 17,
        PROJ_XREPEAT = 18,
        PROJ_YREPEAT = 19,
        PROJ_PAL = 20,
        PROJ_EXTRA_RAND = 21,
        PROJ_HITRADIUS = 22,
        PROJ_VEL_MULT = 23,
        PROJ_OFFSET = 24,
        PROJ_BOUNCES = 25,
        PROJ_BSOUND = 26,
        PROJ_RANGE = 27,
        PROJ_FLASH_COLOR = 28,
    },

    {
        EVENT_INIT = 0,  -- TODO: currently can't run since we init Lunatic state only afterwards
        EVENT_ENTERLEVEL = 1,
        EVENT_RESETWEAPONS = 2,
        EVENT_RESETINVENTORY = 3,
        EVENT_HOLSTER = 4,
        EVENT_LOOKLEFT = 5,
        EVENT_LOOKRIGHT = 6,
        EVENT_SOARUP = 7,
        EVENT_SOARDOWN = 8,
        EVENT_CROUCH = 9,
        EVENT_JUMP = 10,
        EVENT_RETURNTOCENTER = 11,
        EVENT_LOOKUP = 12,
        EVENT_LOOKDOWN = 13,
        EVENT_AIMUP = 14,
        EVENT_FIRE = 15,
        EVENT_CHANGEWEAPON = 16,
        EVENT_GETSHOTRANGE = 17,
        EVENT_GETAUTOAIMANGLE = 18,
        EVENT_GETLOADTILE = 19,
        EVENT_CHEATGETSTEROIDS = 20,
        EVENT_CHEATGETHEAT = 21,
        EVENT_CHEATGETBOOT = 22,
        EVENT_CHEATGETSHIELD = 23,
        EVENT_CHEATGETSCUBA = 24,
        EVENT_CHEATGETHOLODUKE = 25,
        EVENT_CHEATGETJETPACK = 26,
        EVENT_CHEATGETFIRSTAID = 27,
        EVENT_QUICKKICK = 28,
        EVENT_INVENTORY = 29,
        EVENT_USENIGHTVISION = 30,
        EVENT_USESTEROIDS = 31,
        EVENT_INVENTORYLEFT = 32,
        EVENT_INVENTORYRIGHT = 33,
        EVENT_HOLODUKEON = 34,
        EVENT_HOLODUKEOFF = 35,
        EVENT_USEMEDKIT = 36,
        EVENT_USEJETPACK = 37,
        EVENT_TURNAROUND = 38,
        EVENT_DISPLAYWEAPON = 39,
        EVENT_FIREWEAPON = 40,
        EVENT_SELECTWEAPON = 41,
        EVENT_MOVEFORWARD = 42,
        EVENT_MOVEBACKWARD = 43,
        EVENT_TURNLEFT = 44,
        EVENT_TURNRIGHT = 45,
        EVENT_STRAFELEFT = 46,
        EVENT_STRAFERIGHT = 47,
        EVENT_WEAPKEY1 = 48,
        EVENT_WEAPKEY2 = 49,
        EVENT_WEAPKEY3 = 50,
        EVENT_WEAPKEY4 = 51,
        EVENT_WEAPKEY5 = 52,
        EVENT_WEAPKEY6 = 53,
        EVENT_WEAPKEY7 = 54,
        EVENT_WEAPKEY8 = 55,
        EVENT_WEAPKEY9 = 56,
        EVENT_WEAPKEY10 = 57,
        EVENT_DRAWWEAPON = 58,
        EVENT_DISPLAYCROSSHAIR = 59,
        EVENT_DISPLAYREST = 60,
        EVENT_DISPLAYSBAR = 61,
        EVENT_RESETPLAYER = 62,
        EVENT_INCURDAMAGE = 63,
        EVENT_AIMDOWN = 64,
        EVENT_GAME = 65,
        EVENT_PREVIOUSWEAPON = 66,
        EVENT_NEXTWEAPON = 67,
        EVENT_SWIMUP = 68,
        EVENT_SWIMDOWN = 69,
        EVENT_GETMENUTILE = 70,
        EVENT_SPAWN = 71,
        EVENT_LOGO = 72,
        EVENT_EGS = 73,
        EVENT_DOFIRE = 74,
        EVENT_PRESSEDFIRE = 75,
        EVENT_USE = 76,
        EVENT_PROCESSINPUT = 77,
        EVENT_FAKEDOMOVETHINGS = 78,
        EVENT_DISPLAYROOMS = 79,
        EVENT_KILLIT = 80,
        EVENT_LOADACTOR = 81,
        EVENT_DISPLAYBONUSSCREEN = 82,
        EVENT_DISPLAYMENU = 83,
        EVENT_DISPLAYMENUREST = 84,
        EVENT_DISPLAYLOADINGSCREEN = 85,
        EVENT_ANIMATESPRITES = 86,
        EVENT_NEWGAME = 87,
        EVENT_SOUND = 88,
        EVENT_CHECKTOUCHDAMAGE = 89,
        EVENT_CHECKFLOORDAMAGE = 90,
        EVENT_LOADGAME = 91,
        EVENT_SAVEGAME = 92,
        EVENT_PREGAME = 93,
        EVENT_CHANGEMENU = 94,
    },
}


-- NOTE: These MUST be in reverse lexicographical order!
-- Per CON syntax, valid identifiers names are disjunct from keywords,
-- so that a rule like
--      t_identifier = -con_keyword * (sp1 + "[") * t_identifier_all
-- (from the final grammar in lunacon.lua) must match the longest
-- possible keyword name, else the negation might wrongly not fail.

keyword =

lpeg.P(false) +
"}" +
"{" +
"zshootvar" +
"zshoot" +
"xorvarvar" +
"xorvar" +
"writearraytofile" +
"whilevarvarn" +
"whilevarn" +
"wackplayer" +
"userquote" +
"useractor" +
"updatesectorz" +
"updatesector" +
"tossweapon" +
"tip" +
"time" +
"switch" +
"subvarvar" +
"subvar" +
"strength" +
"stopsoundvar" +
"stopsound" +
"stopallsounds" +
"stopactorsound" +
"state" +
"starttrackvar" +
"starttrack" +
"startlevel" +
"ssp" +
"sqrt" +
"spriteshadow" +
"spritepal" +
"spritenvg" +
"spritenoshade" +
"spritenopal" +
"spriteflags" +
"spgetlotag" +
"spgethitag" +
"spawn" +
"soundvar" +
"soundoncevar" +
"soundonce" +
"sound" +
"smaxammo" +
"sleeptime" +
"sizeto" +
"sizeat" +
"sin" +
"showviewunbiased" +
"showview" +
"shootvar" +
"shoot" +
"shiftvarr" +
"shiftvarl" +
"setwall" +
"setvarvar" +
"setvar" +
"setuserdef" +
"settspr" +
"setthisprojectile" +
"setsprite" +
"setsector" +
"setprojectile" +
"setplayervar" +
"setplayerangle" +
"setplayer" +
"setinput" +
"setgamepalette" +
"setgamename" +
"setdefname" +
"setcfgname" +
"setaspect" +
"setarray" +
"setactorvar" +
"setactorsoundpitch" +
"setactorangle" +
"setactor" +
"sectsetinterpolation" +
"sectorofwall" +
"sectgetlotag" +
"sectgethitag" +
"sectclearinterpolation" +
"scriptsize" +
"savenn" +
"savemapstate" +
"savegamevar" +
"save" +
"rotatesprite16" +
"rotatesprite" +
"rotatepoint" +
"return" +
"respawnhitag" +
"resizearray" +
"resetplayer" +
"resetcount" +
"resetactioncount" +
"redefinequote" +
"readgamevar" +
"readarrayfromfile" +
"rayintersect" +
"randvarvar" +
"randvar" +
"quote" +
"quake" +
"qsubstr" +
"qstrncat" +
"qstrlen" +
"qstrcpy" +
"qstrcat" +
"qsprintf" +
"qspawnvar" +
"qspawn" +
"qgetsysstr" +
"pstomp" +
"prevspritestat" +
"prevspritesect" +
"precache" +
"pkick" +
"paper" +
"palfrom" +
"orvarvar" +
"orvar" +
"operatesectors" +
"operaterespawns" +
"operatemasterswitches" +
"operateactivators" +
"operate" +
"onevent" +
"nullop" +
"nextspritestat" +
"nextspritesect" +
"neartag" +
"myosx" +
"myospalx" +
"myospal" +
"myos" +
"music" +
"mulvarvar" +
"mulvar" +
"mulscale" +
"movesprite" +
"move" +
"money" +
"modvarvar" +
"modvar" +
"minitext" +
"mikesnd" +
"mail" +
"lotsofglass" +
"lockplayer" +
"loadmapstate" +
"lineintersect" +
"ldist" +
"killit" +
"jump" +
"insertspriteq" +
"inittimer" +
"includedefault" +
"include" +
"ifwasweapon" +
"ifvarxor" +
"ifvarvarxor" +
"ifvarvaror" +
"ifvarvarn" +
"ifvarvarl" +
"ifvarvarg" +
"ifvarvareither" +
"ifvarvare" +
"ifvarvarand" +
"ifvaror" +
"ifvarn" +
"ifvarl" +
"ifvarg" +
"ifvareither" +
"ifvare" +
"ifvarand" +
"ifstrength" +
"ifsquished" +
"ifspritepal" +
"ifspawnedby" +
"ifsound" +
"ifserver" +
"ifrnd" +
"ifrespawn" +
"ifpinventory" +
"ifphealthl" +
"ifpdistl" +
"ifpdistg" +
"ifp" +
"ifoutside" +
"ifonwater" +
"ifnotmoving" +
"ifnosounds" +
"ifmultiplayer" +
"ifmove" +
"ifinwater" +
"ifinspace" +
"ifinouterspace" +
"ifhitweapon" +
"ifhitspace" +
"ifgotweaponce" +
"ifgapzl" +
"iffloordistl" +
"ifdead" +
"ifcount" +
"ifclient" +
"ifceilingdistl" +
"ifcanshoottarget" +
"ifcanseetarget" +
"ifcansee" +
"ifbulletnear" +
"ifawayfromwall" +
"ifangdiffl" +
"ifai" +
"ifactorsound" +
"ifactornotstayput" +
"ifactor" +
"ifactioncount" +
"ifaction" +
"hitscan" +
"hitradiusvar" +
"hitradius" +
"headspritestat" +
"headspritesect" +
"guts" +
"guniqhudid" +
"gmaxammo" +
"globalsoundvar" +
"globalsound" +
"getzrange" +
"getwall" +
"getuserdef" +
"gettspr" +
"gettimedate" +
"getticks" +
"getthisprojectile" +
"gettexturefloor" +
"gettextureceiling" +
"getsector" +
"getprojectile" +
"getpname" +
"getplayervar" +
"getplayerangle" +
"getplayer" +
"getlastpal" +
"getkeyname" +
"getinput" +
"getincangle" +
"getflorzofslope" +
"getcurraddress" +
"getceilzofslope" +
"getarraysize" +
"getangletotarget" +
"getangle" +
"getactorvar" +
"getactorangle" +
"getactor" +
"gamevar" +
"gametextz" +
"gametext" +
"gamestartup" +
"gamearray" +
"flash" +
"findplayer" +
"findotherplayer" +
"findnearspritezvar" +
"findnearspritez" +
"findnearspritevar" +
"findnearsprite3dvar" +
"findnearsprite3d" +
"findnearsprite" +
"findnearactorzvar" +
"findnearactorz" +
"findnearactorvar" +
"findnearactor3dvar" +
"findnearactor3d" +
"findnearactor" +
"fall" +
"ezshootvar" +
"ezshoot" +
"eventloadactor" +
"espawnvar" +
"espawn" +
"eshootvar" +
"eshoot" +
"eqspawnvar" +
"eqspawn" +
"enhanced" +
"endswitch" +
"ends" +
"endofgame" +
"endevent" +
"enda" +
"else" +
"echo" +
"dynamicremap" +
"dragpoint" +
"divvarvar" +
"divvar" +
"dist" +
"displayrandvarvar" +
"displayrandvar" +
"displayrand" +
"digitalnumberz" +
"digitalnumber" +
"definevolumename" +
"definesound" +
"defineskillname" +
"definequote" +
"defineprojectile" +
"definelevelname" +
"definegametype" +
"definegamefuncname" +
"definecheat" +
"define" +
"default" +
"debug" +
"debris" +
"cstator" +
"cstat" +
"count" +
"cos" +
"copy" +
"cmenu" +
"clipmovenoslide" +
"clipmove" +
"clipdist" +
"clearmapstate" +
"checkavailweapon" +
"checkavailinven" +
"checkactivatormotion" +
"cheatkeys" +
"changespritestat" +
"changespritesect" +
"case" +
"canseespr" +
"cansee" +
"calchypotenuse" +
"cactor" +
"break" +
"betaname" +
"angoffvar" +
"angoff" +
"andvarvar" +
"andvar" +
"ai" +
"addweaponvar" +
"addweapon" +
"addvarvar" +
"addvar" +
"addstrength" +
"addphealth" +
"addlogvar" +
"addlog" +
"addkills" +
"addinventory" +
"addammo" +
"actor" +
"activatecheat" +
"activatebysector" +
"action" +
lpeg.P(false)
