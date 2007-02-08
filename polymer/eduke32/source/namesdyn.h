//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

extern unsigned short SECTOREFFECTOR;
#define SECTOREFFECTOR__STATIC 1
extern unsigned short ACTIVATOR;
#define ACTIVATOR__STATIC 2
extern unsigned short TOUCHPLATE;
#define TOUCHPLATE__STATIC 3
extern unsigned short ACTIVATORLOCKED;
#define ACTIVATORLOCKED__STATIC 4
extern unsigned short MUSICANDSFX;
#define MUSICANDSFX__STATIC 5
extern unsigned short LOCATORS;
#define LOCATORS__STATIC 6
extern unsigned short CYCLER;
#define CYCLER__STATIC 7
extern unsigned short MASTERSWITCH;
#define MASTERSWITCH__STATIC 8
extern unsigned short RESPAWN;
#define RESPAWN__STATIC 9
extern unsigned short GPSPEED;
#define GPSPEED__STATIC 10
extern unsigned short FOF;
#define FOF__STATIC 13
extern unsigned short ARROW;
#define ARROW__STATIC 20
extern unsigned short FIRSTGUNSPRITE;
#define FIRSTGUNSPRITE__STATIC 21
extern unsigned short CHAINGUNSPRITE;
#define CHAINGUNSPRITE__STATIC 22
extern unsigned short RPGSPRITE;
#define RPGSPRITE__STATIC 23
extern unsigned short FREEZESPRITE;
#define FREEZESPRITE__STATIC 24
extern unsigned short SHRINKERSPRITE;
#define SHRINKERSPRITE__STATIC 25
extern unsigned short HEAVYHBOMB;
#define HEAVYHBOMB__STATIC 26
extern unsigned short TRIPBOMBSPRITE;
#define TRIPBOMBSPRITE__STATIC 27
extern unsigned short SHOTGUNSPRITE;
#define SHOTGUNSPRITE__STATIC 28
extern unsigned short DEVISTATORSPRITE;
#define DEVISTATORSPRITE__STATIC 29
extern unsigned short HEALTHBOX;
#define HEALTHBOX__STATIC 30
extern unsigned short AMMOBOX;
#define AMMOBOX__STATIC 31
extern unsigned short GROWSPRITEICON;
#define GROWSPRITEICON__STATIC 32
extern unsigned short INVENTORYBOX;
#define INVENTORYBOX__STATIC 33
extern unsigned short FREEZEAMMO;
#define FREEZEAMMO__STATIC 37
extern unsigned short AMMO;
#define AMMO__STATIC 40
extern unsigned short BATTERYAMMO;
#define BATTERYAMMO__STATIC 41
extern unsigned short DEVISTATORAMMO;
#define DEVISTATORAMMO__STATIC 42
extern unsigned short RPGAMMO;
#define RPGAMMO__STATIC 44
extern unsigned short GROWAMMO;
#define GROWAMMO__STATIC 45
extern unsigned short CRYSTALAMMO;
#define CRYSTALAMMO__STATIC 46
extern unsigned short HBOMBAMMO;
#define HBOMBAMMO__STATIC 47
extern unsigned short AMMOLOTS;
#define AMMOLOTS__STATIC 48
extern unsigned short SHOTGUNAMMO;
#define SHOTGUNAMMO__STATIC 49
extern unsigned short COLA;
#define COLA__STATIC 51
extern unsigned short SIXPAK;
#define SIXPAK__STATIC 52
extern unsigned short FIRSTAID;
#define FIRSTAID__STATIC 53
extern unsigned short SHIELD;
#define SHIELD__STATIC 54
extern unsigned short STEROIDS;
#define STEROIDS__STATIC 55
extern unsigned short AIRTANK;
#define AIRTANK__STATIC 56
extern unsigned short JETPACK;
#define JETPACK__STATIC 57
extern unsigned short HEATSENSOR;
#define HEATSENSOR__STATIC 59
extern unsigned short ACCESSCARD;
#define ACCESSCARD__STATIC 60
extern unsigned short BOOTS;
#define BOOTS__STATIC 61
extern unsigned short MIRRORBROKE;
#define MIRRORBROKE__STATIC 70
extern unsigned short CLOUDYOCEAN;
#define CLOUDYOCEAN__STATIC 78
extern unsigned short CLOUDYSKIES;
#define CLOUDYSKIES__STATIC 79
extern unsigned short MOONSKY1;
#define MOONSKY1__STATIC 80
extern unsigned short MOONSKY2;
#define MOONSKY2__STATIC 81
extern unsigned short MOONSKY3;
#define MOONSKY3__STATIC 82
extern unsigned short MOONSKY4;
#define MOONSKY4__STATIC 83
extern unsigned short BIGORBIT1;
#define BIGORBIT1__STATIC 84
extern unsigned short BIGORBIT2;
#define BIGORBIT2__STATIC 85
extern unsigned short BIGORBIT3;
#define BIGORBIT3__STATIC 86
extern unsigned short BIGORBIT4;
#define BIGORBIT4__STATIC 87
extern unsigned short BIGORBIT5;
#define BIGORBIT5__STATIC 88
extern unsigned short LA;
#define LA__STATIC 89
extern unsigned short REDSKY1;
#define REDSKY1__STATIC 98
extern unsigned short REDSKY2;
#define REDSKY2__STATIC 99
extern unsigned short ATOMICHEALTH;
#define ATOMICHEALTH__STATIC 100
extern unsigned short TECHLIGHT2;
#define TECHLIGHT2__STATIC 120
extern unsigned short TECHLIGHTBUST2;
#define TECHLIGHTBUST2__STATIC 121
extern unsigned short TECHLIGHT4;
#define TECHLIGHT4__STATIC 122
extern unsigned short TECHLIGHTBUST4;
#define TECHLIGHTBUST4__STATIC 123
extern unsigned short WALLLIGHT4;
#define WALLLIGHT4__STATIC 124
extern unsigned short WALLLIGHTBUST4;
#define WALLLIGHTBUST4__STATIC 125
extern unsigned short ACCESSSWITCH;
#define ACCESSSWITCH__STATIC 130
extern unsigned short SLOTDOOR;
#define SLOTDOOR__STATIC 132
extern unsigned short LIGHTSWITCH;
#define LIGHTSWITCH__STATIC 134
extern unsigned short SPACEDOORSWITCH;
#define SPACEDOORSWITCH__STATIC 136
extern unsigned short SPACELIGHTSWITCH;
#define SPACELIGHTSWITCH__STATIC 138
extern unsigned short FRANKENSTINESWITCH;
#define FRANKENSTINESWITCH__STATIC 140
extern unsigned short NUKEBUTTON;
#define NUKEBUTTON__STATIC 142
extern unsigned short MULTISWITCH;
#define MULTISWITCH__STATIC 146
extern unsigned short DOORTILE5;
#define DOORTILE5__STATIC 150
extern unsigned short DOORTILE6;
#define DOORTILE6__STATIC 151
extern unsigned short DOORTILE1;
#define DOORTILE1__STATIC 152
extern unsigned short DOORTILE2;
#define DOORTILE2__STATIC 153
extern unsigned short DOORTILE3;
#define DOORTILE3__STATIC 154
extern unsigned short DOORTILE4;
#define DOORTILE4__STATIC 155
extern unsigned short DOORTILE7;
#define DOORTILE7__STATIC 156
extern unsigned short DOORTILE8;
#define DOORTILE8__STATIC 157
extern unsigned short DOORTILE9;
#define DOORTILE9__STATIC 158
extern unsigned short DOORTILE10;
#define DOORTILE10__STATIC 159
extern unsigned short DOORSHOCK;
#define DOORSHOCK__STATIC 160
extern unsigned short DIPSWITCH;
#define DIPSWITCH__STATIC 162
extern unsigned short DIPSWITCH2;
#define DIPSWITCH2__STATIC 164
extern unsigned short TECHSWITCH;
#define TECHSWITCH__STATIC 166
extern unsigned short DIPSWITCH3;
#define DIPSWITCH3__STATIC 168
extern unsigned short ACCESSSWITCH2;
#define ACCESSSWITCH2__STATIC 170
extern unsigned short REFLECTWATERTILE;
#define REFLECTWATERTILE__STATIC 180
extern unsigned short FLOORSLIME;
#define FLOORSLIME__STATIC 200
extern unsigned short BIGFORCE;
#define BIGFORCE__STATIC 230
extern unsigned short EPISODE;
#define EPISODE__STATIC 247
extern unsigned short MASKWALL9;
#define MASKWALL9__STATIC 255
extern unsigned short W_LIGHT;
#define W_LIGHT__STATIC 260
extern unsigned short SCREENBREAK1;
#define SCREENBREAK1__STATIC 263
extern unsigned short SCREENBREAK2;
#define SCREENBREAK2__STATIC 264
extern unsigned short SCREENBREAK3;
#define SCREENBREAK3__STATIC 265
extern unsigned short SCREENBREAK4;
#define SCREENBREAK4__STATIC 266
extern unsigned short SCREENBREAK5;
#define SCREENBREAK5__STATIC 267
extern unsigned short SCREENBREAK6;
#define SCREENBREAK6__STATIC 268
extern unsigned short SCREENBREAK7;
#define SCREENBREAK7__STATIC 269
extern unsigned short SCREENBREAK8;
#define SCREENBREAK8__STATIC 270
extern unsigned short SCREENBREAK9;
#define SCREENBREAK9__STATIC 271
extern unsigned short SCREENBREAK10;
#define SCREENBREAK10__STATIC 272
extern unsigned short SCREENBREAK11;
#define SCREENBREAK11__STATIC 273
extern unsigned short SCREENBREAK12;
#define SCREENBREAK12__STATIC 274
extern unsigned short SCREENBREAK13;
#define SCREENBREAK13__STATIC 275
extern unsigned short MASKWALL1;
#define MASKWALL1__STATIC 285
extern unsigned short W_TECHWALL1;
#define W_TECHWALL1__STATIC 293
extern unsigned short W_TECHWALL2;
#define W_TECHWALL2__STATIC 297
extern unsigned short W_TECHWALL15;
#define W_TECHWALL15__STATIC 299
extern unsigned short W_TECHWALL3;
#define W_TECHWALL3__STATIC 301
extern unsigned short W_TECHWALL4;
#define W_TECHWALL4__STATIC 305
extern unsigned short W_TECHWALL10;
#define W_TECHWALL10__STATIC 306
extern unsigned short W_TECHWALL16;
#define W_TECHWALL16__STATIC 307
extern unsigned short WATERTILE2;
#define WATERTILE2__STATIC 336
extern unsigned short BPANNEL1;
#define BPANNEL1__STATIC 341
extern unsigned short PANNEL1;
#define PANNEL1__STATIC 342
extern unsigned short PANNEL2;
#define PANNEL2__STATIC 343
extern unsigned short WATERTILE;
#define WATERTILE__STATIC 344
extern unsigned short STATIC;
#define STATIC__STATIC 351
extern unsigned short W_SCREENBREAK;
#define W_SCREENBREAK__STATIC 357
extern unsigned short W_HITTECHWALL3;
#define W_HITTECHWALL3__STATIC 360
extern unsigned short W_HITTECHWALL4;
#define W_HITTECHWALL4__STATIC 361
extern unsigned short W_HITTECHWALL2;
#define W_HITTECHWALL2__STATIC 362
extern unsigned short W_HITTECHWALL1;
#define W_HITTECHWALL1__STATIC 363
extern unsigned short MASKWALL10;
#define MASKWALL10__STATIC 387
extern unsigned short MASKWALL11;
#define MASKWALL11__STATIC 391
extern unsigned short DOORTILE22;
#define DOORTILE22__STATIC 395
extern unsigned short FANSPRITE;
#define FANSPRITE__STATIC 407
extern unsigned short FANSPRITEBROKE;
#define FANSPRITEBROKE__STATIC 411
extern unsigned short FANSHADOW;
#define FANSHADOW__STATIC 412
extern unsigned short FANSHADOWBROKE;
#define FANSHADOWBROKE__STATIC 416
extern unsigned short DOORTILE18;
#define DOORTILE18__STATIC 447
extern unsigned short DOORTILE19;
#define DOORTILE19__STATIC 448
extern unsigned short DOORTILE20;
#define DOORTILE20__STATIC 449
extern unsigned short SATELLITE;
#define SATELLITE__STATIC 489
extern unsigned short VIEWSCREEN2;
#define VIEWSCREEN2__STATIC 499
extern unsigned short VIEWSCREENBROKE;
#define VIEWSCREENBROKE__STATIC 501
extern unsigned short VIEWSCREEN;
#define VIEWSCREEN__STATIC 502
extern unsigned short GLASS;
#define GLASS__STATIC 503
extern unsigned short GLASS2;
#define GLASS2__STATIC 504
extern unsigned short STAINGLASS1;
#define STAINGLASS1__STATIC 510
extern unsigned short MASKWALL5;
#define MASKWALL5__STATIC 514
extern unsigned short SATELITE;
#define SATELITE__STATIC 516
extern unsigned short FUELPOD;
#define FUELPOD__STATIC 517
extern unsigned short SLIMEPIPE;
#define SLIMEPIPE__STATIC 538
extern unsigned short CRACK1;
#define CRACK1__STATIC 546
extern unsigned short CRACK2;
#define CRACK2__STATIC 547
extern unsigned short CRACK3;
#define CRACK3__STATIC 548
extern unsigned short CRACK4;
#define CRACK4__STATIC 549
extern unsigned short FOOTPRINTS;
#define FOOTPRINTS__STATIC 550
extern unsigned short DOMELITE;
#define DOMELITE__STATIC 551
extern unsigned short CAMERAPOLE;
#define CAMERAPOLE__STATIC 554
extern unsigned short CHAIR1;
#define CHAIR1__STATIC 556
extern unsigned short CHAIR2;
#define CHAIR2__STATIC 557
extern unsigned short BROKENCHAIR;
#define BROKENCHAIR__STATIC 559
extern unsigned short MIRROR;
#define MIRROR__STATIC 560
extern unsigned short WATERFOUNTAIN;
#define WATERFOUNTAIN__STATIC 563
extern unsigned short WATERFOUNTAINBROKE;
#define WATERFOUNTAINBROKE__STATIC 567
extern unsigned short FEMMAG1;
#define FEMMAG1__STATIC 568
extern unsigned short TOILET;
#define TOILET__STATIC 569
extern unsigned short STALL;
#define STALL__STATIC 571
extern unsigned short STALLBROKE;
#define STALLBROKE__STATIC 573
extern unsigned short FEMMAG2;
#define FEMMAG2__STATIC 577
extern unsigned short REACTOR2;
#define REACTOR2__STATIC 578
extern unsigned short REACTOR2BURNT;
#define REACTOR2BURNT__STATIC 579
extern unsigned short REACTOR2SPARK;
#define REACTOR2SPARK__STATIC 580
extern unsigned short GRATE1;
#define GRATE1__STATIC 595
extern unsigned short BGRATE1;
#define BGRATE1__STATIC 596
extern unsigned short SOLARPANNEL;
#define SOLARPANNEL__STATIC 602
extern unsigned short NAKED1;
#define NAKED1__STATIC 603
extern unsigned short ANTENNA;
#define ANTENNA__STATIC 607
extern unsigned short MASKWALL12;
#define MASKWALL12__STATIC 609
extern unsigned short TOILETBROKE;
#define TOILETBROKE__STATIC 615
extern unsigned short PIPE2;
#define PIPE2__STATIC 616
extern unsigned short PIPE1B;
#define PIPE1B__STATIC 617
extern unsigned short PIPE3;
#define PIPE3__STATIC 618
extern unsigned short PIPE1;
#define PIPE1__STATIC 619
extern unsigned short CAMERA1;
#define CAMERA1__STATIC 621
extern unsigned short BRICK;
#define BRICK__STATIC 626
extern unsigned short SPLINTERWOOD;
#define SPLINTERWOOD__STATIC 630
extern unsigned short PIPE2B;
#define PIPE2B__STATIC 633
extern unsigned short BOLT1;
#define BOLT1__STATIC 634
extern unsigned short W_NUMBERS;
#define W_NUMBERS__STATIC 640
extern unsigned short WATERDRIP;
#define WATERDRIP__STATIC 660
extern unsigned short WATERBUBBLE;
#define WATERBUBBLE__STATIC 661
extern unsigned short WATERBUBBLEMAKER;
#define WATERBUBBLEMAKER__STATIC 662
extern unsigned short W_FORCEFIELD;
#define W_FORCEFIELD__STATIC 663
extern unsigned short VACUUM;
#define VACUUM__STATIC 669
extern unsigned short FOOTPRINTS2;
#define FOOTPRINTS2__STATIC 672
extern unsigned short FOOTPRINTS3;
#define FOOTPRINTS3__STATIC 673
extern unsigned short FOOTPRINTS4;
#define FOOTPRINTS4__STATIC 674
extern unsigned short EGG;
#define EGG__STATIC 675
extern unsigned short SCALE;
#define SCALE__STATIC 678
extern unsigned short CHAIR3;
#define CHAIR3__STATIC 680
extern unsigned short CAMERALIGHT;
#define CAMERALIGHT__STATIC 685
extern unsigned short MOVIECAMERA;
#define MOVIECAMERA__STATIC 686
extern unsigned short IVUNIT;
#define IVUNIT__STATIC 689
extern unsigned short POT1;
#define POT1__STATIC 694
extern unsigned short POT2;
#define POT2__STATIC 695
extern unsigned short POT3;
#define POT3__STATIC 697
extern unsigned short PIPE3B;
#define PIPE3B__STATIC 700
extern unsigned short WALLLIGHT3;
#define WALLLIGHT3__STATIC 701
extern unsigned short WALLLIGHTBUST3;
#define WALLLIGHTBUST3__STATIC 702
extern unsigned short WALLLIGHT1;
#define WALLLIGHT1__STATIC 703
extern unsigned short WALLLIGHTBUST1;
#define WALLLIGHTBUST1__STATIC 704
extern unsigned short WALLLIGHT2;
#define WALLLIGHT2__STATIC 705
extern unsigned short WALLLIGHTBUST2;
#define WALLLIGHTBUST2__STATIC 706
extern unsigned short LIGHTSWITCH2;
#define LIGHTSWITCH2__STATIC 712
extern unsigned short WAITTOBESEATED;
#define WAITTOBESEATED__STATIC 716
extern unsigned short DOORTILE14;
#define DOORTILE14__STATIC 717
extern unsigned short STATUE;
#define STATUE__STATIC 753
extern unsigned short MIKE;
#define MIKE__STATIC 762
extern unsigned short VASE;
#define VASE__STATIC 765
extern unsigned short SUSHIPLATE1;
#define SUSHIPLATE1__STATIC 768
extern unsigned short SUSHIPLATE2;
#define SUSHIPLATE2__STATIC 769
extern unsigned short SUSHIPLATE3;
#define SUSHIPLATE3__STATIC 774
extern unsigned short SUSHIPLATE4;
#define SUSHIPLATE4__STATIC 779
extern unsigned short DOORTILE16;
#define DOORTILE16__STATIC 781
extern unsigned short SUSHIPLATE5;
#define SUSHIPLATE5__STATIC 792
extern unsigned short OJ;
#define OJ__STATIC 806
extern unsigned short MASKWALL13;
#define MASKWALL13__STATIC 830
extern unsigned short HURTRAIL;
#define HURTRAIL__STATIC 859
extern unsigned short POWERSWITCH1;
#define POWERSWITCH1__STATIC 860
extern unsigned short LOCKSWITCH1;
#define LOCKSWITCH1__STATIC 862
extern unsigned short POWERSWITCH2;
#define POWERSWITCH2__STATIC 864
extern unsigned short ATM;
#define ATM__STATIC 867
extern unsigned short STATUEFLASH;
#define STATUEFLASH__STATIC 869
extern unsigned short ATMBROKE;
#define ATMBROKE__STATIC 888
extern unsigned short BIGHOLE2;
#define BIGHOLE2__STATIC 893
extern unsigned short STRIPEBALL;
#define STRIPEBALL__STATIC 901
extern unsigned short QUEBALL;
#define QUEBALL__STATIC 902
extern unsigned short POCKET;
#define POCKET__STATIC 903
extern unsigned short WOODENHORSE;
#define WOODENHORSE__STATIC 904
extern unsigned short TREE1;
#define TREE1__STATIC 908
extern unsigned short TREE2;
#define TREE2__STATIC 910
extern unsigned short CACTUS;
#define CACTUS__STATIC 911
extern unsigned short MASKWALL2;
#define MASKWALL2__STATIC 913
extern unsigned short MASKWALL3;
#define MASKWALL3__STATIC 914
extern unsigned short MASKWALL4;
#define MASKWALL4__STATIC 915
extern unsigned short FIREEXT;
#define FIREEXT__STATIC 916
extern unsigned short TOILETWATER;
#define TOILETWATER__STATIC 921
extern unsigned short NEON1;
#define NEON1__STATIC 925
extern unsigned short NEON2;
#define NEON2__STATIC 926
extern unsigned short CACTUSBROKE;
#define CACTUSBROKE__STATIC 939
extern unsigned short BOUNCEMINE;
#define BOUNCEMINE__STATIC 940
extern unsigned short BROKEFIREHYDRENT;
#define BROKEFIREHYDRENT__STATIC 950
extern unsigned short BOX;
#define BOX__STATIC 951
extern unsigned short BULLETHOLE;
#define BULLETHOLE__STATIC 952
extern unsigned short BOTTLE1;
#define BOTTLE1__STATIC 954
extern unsigned short BOTTLE2;
#define BOTTLE2__STATIC 955
extern unsigned short BOTTLE3;
#define BOTTLE3__STATIC 956
extern unsigned short BOTTLE4;
#define BOTTLE4__STATIC 957
extern unsigned short FEMPIC5;
#define FEMPIC5__STATIC 963
extern unsigned short FEMPIC6;
#define FEMPIC6__STATIC 964
extern unsigned short FEMPIC7;
#define FEMPIC7__STATIC 965
extern unsigned short HYDROPLANT;
#define HYDROPLANT__STATIC 969
extern unsigned short OCEANSPRITE1;
#define OCEANSPRITE1__STATIC 971
extern unsigned short OCEANSPRITE2;
#define OCEANSPRITE2__STATIC 972
extern unsigned short OCEANSPRITE3;
#define OCEANSPRITE3__STATIC 973
extern unsigned short OCEANSPRITE4;
#define OCEANSPRITE4__STATIC 974
extern unsigned short OCEANSPRITE5;
#define OCEANSPRITE5__STATIC 975
extern unsigned short GENERICPOLE;
#define GENERICPOLE__STATIC 977
extern unsigned short CONE;
#define CONE__STATIC 978
extern unsigned short HANGLIGHT;
#define HANGLIGHT__STATIC 979
extern unsigned short HYDRENT;
#define HYDRENT__STATIC 981
extern unsigned short MASKWALL14;
#define MASKWALL14__STATIC 988
extern unsigned short TIRE;
#define TIRE__STATIC 990
extern unsigned short PIPE5;
#define PIPE5__STATIC 994
extern unsigned short PIPE6;
#define PIPE6__STATIC 995
extern unsigned short PIPE4;
#define PIPE4__STATIC 996
extern unsigned short PIPE4B;
#define PIPE4B__STATIC 997
extern unsigned short BROKEHYDROPLANT;
#define BROKEHYDROPLANT__STATIC 1003
extern unsigned short PIPE5B;
#define PIPE5B__STATIC 1005
extern unsigned short NEON3;
#define NEON3__STATIC 1007
extern unsigned short NEON4;
#define NEON4__STATIC 1008
extern unsigned short NEON5;
#define NEON5__STATIC 1009
extern unsigned short BOTTLE5;
#define BOTTLE5__STATIC 1012
extern unsigned short BOTTLE6;
#define BOTTLE6__STATIC 1013
extern unsigned short BOTTLE8;
#define BOTTLE8__STATIC 1014
extern unsigned short SPOTLITE;
#define SPOTLITE__STATIC 1020
extern unsigned short HANGOOZ;
#define HANGOOZ__STATIC 1022
extern unsigned short MASKWALL15;
#define MASKWALL15__STATIC 1024
extern unsigned short BOTTLE7;
#define BOTTLE7__STATIC 1025
extern unsigned short HORSEONSIDE;
#define HORSEONSIDE__STATIC 1026
extern unsigned short GLASSPIECES;
#define GLASSPIECES__STATIC 1031
extern unsigned short HORSELITE;
#define HORSELITE__STATIC 1034
extern unsigned short DONUTS;
#define DONUTS__STATIC 1045
extern unsigned short NEON6;
#define NEON6__STATIC 1046
extern unsigned short MASKWALL6;
#define MASKWALL6__STATIC 1059
extern unsigned short CLOCK;
#define CLOCK__STATIC 1060
extern unsigned short RUBBERCAN;
#define RUBBERCAN__STATIC 1062
extern unsigned short BROKENCLOCK;
#define BROKENCLOCK__STATIC 1067
extern unsigned short PLUG;
#define PLUG__STATIC 1069
extern unsigned short OOZFILTER;
#define OOZFILTER__STATIC 1079
extern unsigned short FLOORPLASMA;
#define FLOORPLASMA__STATIC 1082
extern unsigned short REACTOR;
#define REACTOR__STATIC 1088
extern unsigned short REACTORSPARK;
#define REACTORSPARK__STATIC 1092
extern unsigned short REACTORBURNT;
#define REACTORBURNT__STATIC 1096
extern unsigned short DOORTILE15;
#define DOORTILE15__STATIC 1102
extern unsigned short HANDSWITCH;
#define HANDSWITCH__STATIC 1111
extern unsigned short CIRCLEPANNEL;
#define CIRCLEPANNEL__STATIC 1113
extern unsigned short CIRCLEPANNELBROKE;
#define CIRCLEPANNELBROKE__STATIC 1114
extern unsigned short PULLSWITCH;
#define PULLSWITCH__STATIC 1122
extern unsigned short MASKWALL8;
#define MASKWALL8__STATIC 1124
extern unsigned short BIGHOLE;
#define BIGHOLE__STATIC 1141
extern unsigned short ALIENSWITCH;
#define ALIENSWITCH__STATIC 1142
extern unsigned short DOORTILE21;
#define DOORTILE21__STATIC 1144
extern unsigned short HANDPRINTSWITCH;
#define HANDPRINTSWITCH__STATIC 1155
extern unsigned short BOTTLE10;
#define BOTTLE10__STATIC 1157
extern unsigned short BOTTLE11;
#define BOTTLE11__STATIC 1158
extern unsigned short BOTTLE12;
#define BOTTLE12__STATIC 1159
extern unsigned short BOTTLE13;
#define BOTTLE13__STATIC 1160
extern unsigned short BOTTLE14;
#define BOTTLE14__STATIC 1161
extern unsigned short BOTTLE15;
#define BOTTLE15__STATIC 1162
extern unsigned short BOTTLE16;
#define BOTTLE16__STATIC 1163
extern unsigned short BOTTLE17;
#define BOTTLE17__STATIC 1164
extern unsigned short BOTTLE18;
#define BOTTLE18__STATIC 1165
extern unsigned short BOTTLE19;
#define BOTTLE19__STATIC 1166
extern unsigned short DOORTILE17;
#define DOORTILE17__STATIC 1169
extern unsigned short MASKWALL7;
#define MASKWALL7__STATIC 1174
extern unsigned short JAILBARBREAK;
#define JAILBARBREAK__STATIC 1175
extern unsigned short DOORTILE11;
#define DOORTILE11__STATIC 1178
extern unsigned short DOORTILE12;
#define DOORTILE12__STATIC 1179
extern unsigned short VENDMACHINE;
#define VENDMACHINE__STATIC 1212
extern unsigned short VENDMACHINEBROKE;
#define VENDMACHINEBROKE__STATIC 1214
extern unsigned short COLAMACHINE;
#define COLAMACHINE__STATIC 1215
extern unsigned short COLAMACHINEBROKE;
#define COLAMACHINEBROKE__STATIC 1217
extern unsigned short CRANEPOLE;
#define CRANEPOLE__STATIC 1221
extern unsigned short CRANE;
#define CRANE__STATIC 1222
extern unsigned short BARBROKE;
#define BARBROKE__STATIC 1225
extern unsigned short BLOODPOOL;
#define BLOODPOOL__STATIC 1226
extern unsigned short NUKEBARREL;
#define NUKEBARREL__STATIC 1227
extern unsigned short NUKEBARRELDENTED;
#define NUKEBARRELDENTED__STATIC 1228
extern unsigned short NUKEBARRELLEAKED;
#define NUKEBARRELLEAKED__STATIC 1229
extern unsigned short CANWITHSOMETHING;
#define CANWITHSOMETHING__STATIC 1232
extern unsigned short MONEY;
#define MONEY__STATIC 1233
extern unsigned short BANNER;
#define BANNER__STATIC 1236
extern unsigned short EXPLODINGBARREL;
#define EXPLODINGBARREL__STATIC 1238
extern unsigned short EXPLODINGBARREL2;
#define EXPLODINGBARREL2__STATIC 1239
extern unsigned short FIREBARREL;
#define FIREBARREL__STATIC 1240
extern unsigned short SEENINE;
#define SEENINE__STATIC 1247
extern unsigned short SEENINEDEAD;
#define SEENINEDEAD__STATIC 1248
extern unsigned short STEAM;
#define STEAM__STATIC 1250
extern unsigned short CEILINGSTEAM;
#define CEILINGSTEAM__STATIC 1255
extern unsigned short PIPE6B;
#define PIPE6B__STATIC 1260
extern unsigned short TRANSPORTERBEAM;
#define TRANSPORTERBEAM__STATIC 1261
extern unsigned short RAT;
#define RAT__STATIC 1267
extern unsigned short TRASH;
#define TRASH__STATIC 1272
extern unsigned short FEMPIC1;
#define FEMPIC1__STATIC 1280
extern unsigned short FEMPIC2;
#define FEMPIC2__STATIC 1289
extern unsigned short BLANKSCREEN;
#define BLANKSCREEN__STATIC 1293
extern unsigned short PODFEM1;
#define PODFEM1__STATIC 1294
extern unsigned short FEMPIC3;
#define FEMPIC3__STATIC 1298
extern unsigned short FEMPIC4;
#define FEMPIC4__STATIC 1306
extern unsigned short FEM1;
#define FEM1__STATIC 1312
extern unsigned short FEM2;
#define FEM2__STATIC 1317
extern unsigned short FEM3;
#define FEM3__STATIC 1321
extern unsigned short FEM5;
#define FEM5__STATIC 1323
extern unsigned short BLOODYPOLE;
#define BLOODYPOLE__STATIC 1324
extern unsigned short FEM4;
#define FEM4__STATIC 1325
extern unsigned short FEM6;
#define FEM6__STATIC 1334
extern unsigned short FEM6PAD;
#define FEM6PAD__STATIC 1335
extern unsigned short FEM8;
#define FEM8__STATIC 1336
extern unsigned short HELECOPT;
#define HELECOPT__STATIC 1346
extern unsigned short FETUSJIB;
#define FETUSJIB__STATIC 1347
extern unsigned short HOLODUKE;
#define HOLODUKE__STATIC 1348
extern unsigned short SPACEMARINE;
#define SPACEMARINE__STATIC 1353
extern unsigned short INDY;
#define INDY__STATIC 1355
extern unsigned short FETUS;
#define FETUS__STATIC 1358
extern unsigned short FETUSBROKE;
#define FETUSBROKE__STATIC 1359
extern unsigned short MONK;
#define MONK__STATIC 1352
extern unsigned short LUKE;
#define LUKE__STATIC 1354
extern unsigned short COOLEXPLOSION1;
#define COOLEXPLOSION1__STATIC 1360
extern unsigned short WATERSPLASH2;
#define WATERSPLASH2__STATIC 1380
extern unsigned short FIREVASE;
#define FIREVASE__STATIC 1390
extern unsigned short SCRATCH;
#define SCRATCH__STATIC 1393
extern unsigned short FEM7;
#define FEM7__STATIC 1395
extern unsigned short APLAYERTOP;
#define APLAYERTOP__STATIC 1400
extern unsigned short APLAYER;
#define APLAYER__STATIC 1405
extern unsigned short PLAYERONWATER;
#define PLAYERONWATER__STATIC 1420
extern unsigned short DUKELYINGDEAD;
#define DUKELYINGDEAD__STATIC 1518
extern unsigned short DUKETORSO;
#define DUKETORSO__STATIC 1520
extern unsigned short DUKEGUN;
#define DUKEGUN__STATIC 1528
extern unsigned short DUKELEG;
#define DUKELEG__STATIC 1536
extern unsigned short SHARK;
#define SHARK__STATIC 1550
extern unsigned short BLOOD;
#define BLOOD__STATIC 1620
extern unsigned short FIRELASER;
#define FIRELASER__STATIC 1625
extern unsigned short TRANSPORTERSTAR;
#define TRANSPORTERSTAR__STATIC 1630
extern unsigned short SPIT;
#define SPIT__STATIC 1636
extern unsigned short LOOGIE;
#define LOOGIE__STATIC 1637
extern unsigned short FIST;
#define FIST__STATIC 1640
extern unsigned short FREEZEBLAST;
#define FREEZEBLAST__STATIC 1641
extern unsigned short DEVISTATORBLAST;
#define DEVISTATORBLAST__STATIC 1642
extern unsigned short SHRINKSPARK;
#define SHRINKSPARK__STATIC 1646
extern unsigned short TONGUE;
#define TONGUE__STATIC 1647
extern unsigned short MORTER;
#define MORTER__STATIC 1650
extern unsigned short SHRINKEREXPLOSION;
#define SHRINKEREXPLOSION__STATIC 1656
extern unsigned short RADIUSEXPLOSION;
#define RADIUSEXPLOSION__STATIC 1670
extern unsigned short FORCERIPPLE;
#define FORCERIPPLE__STATIC 1671
extern unsigned short LIZTROOP;
#define LIZTROOP__STATIC 1680
extern unsigned short LIZTROOPRUNNING;
#define LIZTROOPRUNNING__STATIC 1681
extern unsigned short LIZTROOPSTAYPUT;
#define LIZTROOPSTAYPUT__STATIC 1682
extern unsigned short LIZTOP;
#define LIZTOP__STATIC 1705
extern unsigned short LIZTROOPSHOOT;
#define LIZTROOPSHOOT__STATIC 1715
extern unsigned short LIZTROOPJETPACK;
#define LIZTROOPJETPACK__STATIC 1725
extern unsigned short LIZTROOPDSPRITE;
#define LIZTROOPDSPRITE__STATIC 1734
extern unsigned short LIZTROOPONTOILET;
#define LIZTROOPONTOILET__STATIC 1741
extern unsigned short LIZTROOPJUSTSIT;
#define LIZTROOPJUSTSIT__STATIC 1742
extern unsigned short LIZTROOPDUCKING;
#define LIZTROOPDUCKING__STATIC 1744
extern unsigned short HEADJIB1;
#define HEADJIB1__STATIC 1768
extern unsigned short ARMJIB1;
#define ARMJIB1__STATIC 1772
extern unsigned short LEGJIB1;
#define LEGJIB1__STATIC 1776
extern unsigned short CANNONBALL;
#define CANNONBALL__STATIC 1817
extern unsigned short OCTABRAIN;
#define OCTABRAIN__STATIC 1820
extern unsigned short OCTABRAINSTAYPUT;
#define OCTABRAINSTAYPUT__STATIC 1821
extern unsigned short OCTATOP;
#define OCTATOP__STATIC 1845
extern unsigned short OCTADEADSPRITE;
#define OCTADEADSPRITE__STATIC 1855
extern unsigned short INNERJAW;
#define INNERJAW__STATIC 1860
extern unsigned short DRONE;
#define DRONE__STATIC 1880
extern unsigned short EXPLOSION2;
#define EXPLOSION2__STATIC 1890
extern unsigned short COMMANDER;
#define COMMANDER__STATIC 1920
extern unsigned short COMMANDERSTAYPUT;
#define COMMANDERSTAYPUT__STATIC 1921
extern unsigned short RECON;
#define RECON__STATIC 1960
extern unsigned short TANK;
#define TANK__STATIC 1975
extern unsigned short PIGCOP;
#define PIGCOP__STATIC 2000
extern unsigned short PIGCOPSTAYPUT;
#define PIGCOPSTAYPUT__STATIC 2001
extern unsigned short PIGCOPDIVE;
#define PIGCOPDIVE__STATIC 2045
extern unsigned short PIGCOPDEADSPRITE;
#define PIGCOPDEADSPRITE__STATIC 2060
extern unsigned short PIGTOP;
#define PIGTOP__STATIC 2061
extern unsigned short LIZMAN;
#define LIZMAN__STATIC 2120
extern unsigned short LIZMANSTAYPUT;
#define LIZMANSTAYPUT__STATIC 2121
extern unsigned short LIZMANSPITTING;
#define LIZMANSPITTING__STATIC 2150
extern unsigned short LIZMANFEEDING;
#define LIZMANFEEDING__STATIC 2160
extern unsigned short LIZMANJUMP;
#define LIZMANJUMP__STATIC 2165
extern unsigned short LIZMANDEADSPRITE;
#define LIZMANDEADSPRITE__STATIC 2185
extern unsigned short FECES;
#define FECES__STATIC 2200
extern unsigned short LIZMANHEAD1;
#define LIZMANHEAD1__STATIC 2201
extern unsigned short LIZMANARM1;
#define LIZMANARM1__STATIC 2205
extern unsigned short LIZMANLEG1;
#define LIZMANLEG1__STATIC 2209
extern unsigned short EXPLOSION2BOT;
#define EXPLOSION2BOT__STATIC 2219
extern unsigned short USERWEAPON;
#define USERWEAPON__STATIC 2235
extern unsigned short HEADERBAR;
#define HEADERBAR__STATIC 2242
extern unsigned short JIBS1;
#define JIBS1__STATIC 2245
extern unsigned short JIBS2;
#define JIBS2__STATIC 2250
extern unsigned short JIBS3;
#define JIBS3__STATIC 2255
extern unsigned short JIBS4;
#define JIBS4__STATIC 2260
extern unsigned short JIBS5;
#define JIBS5__STATIC 2265
extern unsigned short BURNING;
#define BURNING__STATIC 2270
extern unsigned short FIRE;
#define FIRE__STATIC 2271
extern unsigned short JIBS6;
#define JIBS6__STATIC 2286
extern unsigned short BLOODSPLAT1;
#define BLOODSPLAT1__STATIC 2296
extern unsigned short BLOODSPLAT3;
#define BLOODSPLAT3__STATIC 2297
extern unsigned short BLOODSPLAT2;
#define BLOODSPLAT2__STATIC 2298
extern unsigned short BLOODSPLAT4;
#define BLOODSPLAT4__STATIC 2299
extern unsigned short OOZ;
#define OOZ__STATIC 2300
extern unsigned short OOZ2;
#define OOZ2__STATIC 2309
extern unsigned short WALLBLOOD1;
#define WALLBLOOD1__STATIC 2301
extern unsigned short WALLBLOOD2;
#define WALLBLOOD2__STATIC 2302
extern unsigned short WALLBLOOD3;
#define WALLBLOOD3__STATIC 2303
extern unsigned short WALLBLOOD4;
#define WALLBLOOD4__STATIC 2304
extern unsigned short WALLBLOOD5;
#define WALLBLOOD5__STATIC 2305
extern unsigned short WALLBLOOD6;
#define WALLBLOOD6__STATIC 2306
extern unsigned short WALLBLOOD7;
#define WALLBLOOD7__STATIC 2307
extern unsigned short WALLBLOOD8;
#define WALLBLOOD8__STATIC 2308
extern unsigned short BURNING2;
#define BURNING2__STATIC 2310
extern unsigned short FIRE2;
#define FIRE2__STATIC 2311
extern unsigned short CRACKKNUCKLES;
#define CRACKKNUCKLES__STATIC 2324
extern unsigned short SMALLSMOKE;
#define SMALLSMOKE__STATIC 2329
extern unsigned short SMALLSMOKEMAKER;
#define SMALLSMOKEMAKER__STATIC 2330
extern unsigned short FLOORFLAME;
#define FLOORFLAME__STATIC 2333
extern unsigned short ROTATEGUN;
#define ROTATEGUN__STATIC 2360
extern unsigned short GREENSLIME;
#define GREENSLIME__STATIC 2370
extern unsigned short WATERDRIPSPLASH;
#define WATERDRIPSPLASH__STATIC 2380
extern unsigned short SCRAP6;
#define SCRAP6__STATIC 2390
extern unsigned short SCRAP1;
#define SCRAP1__STATIC 2400
extern unsigned short SCRAP2;
#define SCRAP2__STATIC 2404
extern unsigned short SCRAP3;
#define SCRAP3__STATIC 2408
extern unsigned short SCRAP4;
#define SCRAP4__STATIC 2412
extern unsigned short SCRAP5;
#define SCRAP5__STATIC 2416
extern unsigned short ORGANTIC;
#define ORGANTIC__STATIC 2420
extern unsigned short BETAVERSION;
#define BETAVERSION__STATIC 2440
extern unsigned short PLAYERISHERE;
#define PLAYERISHERE__STATIC 2442
extern unsigned short PLAYERWASHERE;
#define PLAYERWASHERE__STATIC 2443
extern unsigned short SELECTDIR;
#define SELECTDIR__STATIC 2444
extern unsigned short F1HELP;
#define F1HELP__STATIC 2445
extern unsigned short NOTCHON;
#define NOTCHON__STATIC 2446
extern unsigned short NOTCHOFF;
#define NOTCHOFF__STATIC 2447
extern unsigned short GROWSPARK;
#define GROWSPARK__STATIC 2448
extern unsigned short DUKEICON;
#define DUKEICON__STATIC 2452
extern unsigned short BADGUYICON;
#define BADGUYICON__STATIC 2453
extern unsigned short FOODICON;
#define FOODICON__STATIC 2454
extern unsigned short GETICON;
#define GETICON__STATIC 2455
extern unsigned short MENUSCREEN;
#define MENUSCREEN__STATIC 2456
extern unsigned short MENUBAR;
#define MENUBAR__STATIC 2457
extern unsigned short KILLSICON;
#define KILLSICON__STATIC 2458
extern unsigned short FIRSTAID_ICON;
#define FIRSTAID_ICON__STATIC 2460
extern unsigned short HEAT_ICON;
#define HEAT_ICON__STATIC 2461
extern unsigned short BOTTOMSTATUSBAR;
#define BOTTOMSTATUSBAR__STATIC 2462
extern unsigned short BOOT_ICON;
#define BOOT_ICON__STATIC 2463
extern unsigned short FRAGBAR;
#define FRAGBAR__STATIC 2465
extern unsigned short JETPACK_ICON;
#define JETPACK_ICON__STATIC 2467
extern unsigned short AIRTANK_ICON;
#define AIRTANK_ICON__STATIC 2468
extern unsigned short STEROIDS_ICON;
#define STEROIDS_ICON__STATIC 2469
extern unsigned short HOLODUKE_ICON;
#define HOLODUKE_ICON__STATIC 2470
extern unsigned short ACCESS_ICON;
#define ACCESS_ICON__STATIC 2471
extern unsigned short DIGITALNUM;
#define DIGITALNUM__STATIC 2472
extern unsigned short DUKECAR;
#define DUKECAR__STATIC 2491
extern unsigned short CAMCORNER;
#define CAMCORNER__STATIC 2482
extern unsigned short CAMLIGHT;
#define CAMLIGHT__STATIC 2484
extern unsigned short LOGO;
#define LOGO__STATIC 2485
extern unsigned short TITLE;
#define TITLE__STATIC 2486
extern unsigned short NUKEWARNINGICON;
#define NUKEWARNINGICON__STATIC 2487
extern unsigned short MOUSECURSOR;
#define MOUSECURSOR__STATIC 2488
extern unsigned short SLIDEBAR;
#define SLIDEBAR__STATIC 2489
extern unsigned short DREALMS;
#define DREALMS__STATIC 2492
extern unsigned short BETASCREEN;
#define BETASCREEN__STATIC 2493
extern unsigned short WINDOWBORDER1;
#define WINDOWBORDER1__STATIC 2494
extern unsigned short TEXTBOX;
#define TEXTBOX__STATIC 2495
extern unsigned short WINDOWBORDER2;
#define WINDOWBORDER2__STATIC 2496
extern unsigned short DUKENUKEM;
#define DUKENUKEM__STATIC 2497
extern unsigned short THREEDEE;
#define THREEDEE__STATIC 2498
extern unsigned short INGAMEDUKETHREEDEE;
#define INGAMEDUKETHREEDEE__STATIC 2499
extern unsigned short TENSCREEN;
#define TENSCREEN__STATIC 2500
extern unsigned short PLUTOPAKSPRITE;
#define PLUTOPAKSPRITE__STATIC 2501
extern unsigned short DEVISTATOR;
#define DEVISTATOR__STATIC 2510
extern unsigned short KNEE;
#define KNEE__STATIC 2521
extern unsigned short CROSSHAIR;
#define CROSSHAIR__STATIC 2523
extern unsigned short FIRSTGUN;
#define FIRSTGUN__STATIC 2524
extern unsigned short FIRSTGUNRELOAD;
#define FIRSTGUNRELOAD__STATIC 2528
extern unsigned short FALLINGCLIP;
#define FALLINGCLIP__STATIC 2530
extern unsigned short CLIPINHAND;
#define CLIPINHAND__STATIC 2531
extern unsigned short HAND;
#define HAND__STATIC 2532
extern unsigned short SHELL;
#define SHELL__STATIC 2533
extern unsigned short SHOTGUNSHELL;
#define SHOTGUNSHELL__STATIC 2535
extern unsigned short CHAINGUN;
#define CHAINGUN__STATIC 2536
extern unsigned short RPGGUN;
#define RPGGUN__STATIC 2544
extern unsigned short RPGMUZZLEFLASH;
#define RPGMUZZLEFLASH__STATIC 2545
extern unsigned short FREEZE;
#define FREEZE__STATIC 2548
extern unsigned short CATLITE;
#define CATLITE__STATIC 2552
extern unsigned short SHRINKER;
#define SHRINKER__STATIC 2556
extern unsigned short HANDHOLDINGLASER;
#define HANDHOLDINGLASER__STATIC 2563
extern unsigned short TRIPBOMB;
#define TRIPBOMB__STATIC 2566
extern unsigned short LASERLINE;
#define LASERLINE__STATIC 2567
extern unsigned short HANDHOLDINGACCESS;
#define HANDHOLDINGACCESS__STATIC 2568
extern unsigned short HANDREMOTE;
#define HANDREMOTE__STATIC 2570
extern unsigned short HANDTHROW;
#define HANDTHROW__STATIC 2573
extern unsigned short TIP;
#define TIP__STATIC 2576
extern unsigned short GLAIR;
#define GLAIR__STATIC 2578
extern unsigned short SCUBAMASK;
#define SCUBAMASK__STATIC 2581
extern unsigned short SPACEMASK;
#define SPACEMASK__STATIC 2584
extern unsigned short FORCESPHERE;
#define FORCESPHERE__STATIC 2590
extern unsigned short SHOTSPARK1;
#define SHOTSPARK1__STATIC 2595
extern unsigned short RPG;
#define RPG__STATIC 2605
extern unsigned short LASERSITE;
#define LASERSITE__STATIC 2612
extern unsigned short SHOTGUN;
#define SHOTGUN__STATIC 2613
extern unsigned short BOSS1;
#define BOSS1__STATIC 2630
extern unsigned short BOSS1STAYPUT;
#define BOSS1STAYPUT__STATIC 2631
extern unsigned short BOSS1SHOOT;
#define BOSS1SHOOT__STATIC 2660
extern unsigned short BOSS1LOB;
#define BOSS1LOB__STATIC 2670
extern unsigned short BOSSTOP;
#define BOSSTOP__STATIC 2696
extern unsigned short BOSS2;
#define BOSS2__STATIC 2710
extern unsigned short BOSS3;
#define BOSS3__STATIC 2760
extern unsigned short SPINNINGNUKEICON;
#define SPINNINGNUKEICON__STATIC 2813
extern unsigned short BIGFNTCURSOR;
#define BIGFNTCURSOR__STATIC 2820
extern unsigned short SMALLFNTCURSOR;
#define SMALLFNTCURSOR__STATIC 2821
extern unsigned short STARTALPHANUM;
#define STARTALPHANUM__STATIC 2822
extern unsigned short ENDALPHANUM;
#define ENDALPHANUM__STATIC 2915
extern unsigned short BIGALPHANUM;
#define BIGALPHANUM__STATIC 2940
extern unsigned short BIGPERIOD;
#define BIGPERIOD__STATIC 3002
extern unsigned short BIGCOMMA;
#define BIGCOMMA__STATIC 3003
extern unsigned short BIGX;
#define BIGX__STATIC 3004
extern unsigned short BIGQ;
#define BIGQ__STATIC 3005
extern unsigned short BIGSEMI;
#define BIGSEMI__STATIC 3006
extern unsigned short BIGCOLIN;
#define BIGCOLIN__STATIC 3007
extern unsigned short THREEBYFIVE;
#define THREEBYFIVE__STATIC 3010
extern unsigned short BIGAPPOS;
#define BIGAPPOS__STATIC 3022
extern unsigned short BLANK;
#define BLANK__STATIC 3026
extern unsigned short MINIFONT;
#define MINIFONT__STATIC 3072
extern unsigned short BUTTON1;
#define BUTTON1__STATIC 3164
extern unsigned short GLASS3;
#define GLASS3__STATIC 3187
extern unsigned short RESPAWNMARKERRED;
#define RESPAWNMARKERRED__STATIC 3190
extern unsigned short RESPAWNMARKERYELLOW;
#define RESPAWNMARKERYELLOW__STATIC 3200
extern unsigned short RESPAWNMARKERGREEN;
#define RESPAWNMARKERGREEN__STATIC 3210
extern unsigned short BONUSSCREEN;
#define BONUSSCREEN__STATIC 3240
extern unsigned short VIEWBORDER;
#define VIEWBORDER__STATIC 3250
extern unsigned short VICTORY1;
#define VICTORY1__STATIC 3260
extern unsigned short ORDERING;
#define ORDERING__STATIC 3270
extern unsigned short TEXTSTORY;
#define TEXTSTORY__STATIC 3280
extern unsigned short LOADSCREEN;
#define LOADSCREEN__STATIC 3281
extern unsigned short BORNTOBEWILDSCREEN;
#define BORNTOBEWILDSCREEN__STATIC 3370
extern unsigned short BLIMP;
#define BLIMP__STATIC 3400
extern unsigned short FEM9;
#define FEM9__STATIC 3450
extern unsigned short FOOTPRINT;
#define FOOTPRINT__STATIC 3701
extern unsigned short FRAMEEFFECT1_13;
#define FRAMEEFFECT1_13__STATIC 3999
extern unsigned short POOP;
#define POOP__STATIC 4094
extern unsigned short FRAMEEFFECT1;
#define FRAMEEFFECT1__STATIC 4095
extern unsigned short PANNEL3;
#define PANNEL3__STATIC 4099
extern unsigned short SCREENBREAK14;
#define SCREENBREAK14__STATIC 4120
extern unsigned short SCREENBREAK15;
#define SCREENBREAK15__STATIC 4123
extern unsigned short SCREENBREAK19;
#define SCREENBREAK19__STATIC 4125
extern unsigned short SCREENBREAK16;
#define SCREENBREAK16__STATIC 4127
extern unsigned short SCREENBREAK17;
#define SCREENBREAK17__STATIC 4128
extern unsigned short SCREENBREAK18;
#define SCREENBREAK18__STATIC 4129
extern unsigned short W_TECHWALL11;
#define W_TECHWALL11__STATIC 4130
extern unsigned short W_TECHWALL12;
#define W_TECHWALL12__STATIC 4131
extern unsigned short W_TECHWALL13;
#define W_TECHWALL13__STATIC 4132
extern unsigned short W_TECHWALL14;
#define W_TECHWALL14__STATIC 4133
extern unsigned short W_TECHWALL5;
#define W_TECHWALL5__STATIC 4134
extern unsigned short W_TECHWALL6;
#define W_TECHWALL6__STATIC 4136
extern unsigned short W_TECHWALL7;
#define W_TECHWALL7__STATIC 4138
extern unsigned short W_TECHWALL8;
#define W_TECHWALL8__STATIC 4140
extern unsigned short W_TECHWALL9;
#define W_TECHWALL9__STATIC 4142
extern unsigned short BPANNEL3;
#define BPANNEL3__STATIC 4100
extern unsigned short W_HITTECHWALL16;
#define W_HITTECHWALL16__STATIC 4144
extern unsigned short W_HITTECHWALL10;
#define W_HITTECHWALL10__STATIC 4145
extern unsigned short W_HITTECHWALL15;
#define W_HITTECHWALL15__STATIC 4147
extern unsigned short W_MILKSHELF;
#define W_MILKSHELF__STATIC 4181
extern unsigned short W_MILKSHELFBROKE;
#define W_MILKSHELFBROKE__STATIC 4203
extern unsigned short PURPLELAVA;
#define PURPLELAVA__STATIC 4240
extern unsigned short LAVABUBBLE;
#define LAVABUBBLE__STATIC 4340
extern unsigned short DUKECUTOUT;
#define DUKECUTOUT__STATIC 4352
extern unsigned short TARGET;
#define TARGET__STATIC 4359
extern unsigned short GUNPOWDERBARREL;
#define GUNPOWDERBARREL__STATIC 4360
extern unsigned short DUCK;
#define DUCK__STATIC 4361
extern unsigned short HATRACK;
#define HATRACK__STATIC 4367
extern unsigned short DESKLAMP;
#define DESKLAMP__STATIC 4370
extern unsigned short COFFEEMACHINE;
#define COFFEEMACHINE__STATIC 4372
extern unsigned short CUPS;
#define CUPS__STATIC 4373
extern unsigned short GAVALS;
#define GAVALS__STATIC 4374
extern unsigned short GAVALS2;
#define GAVALS2__STATIC 4375
extern unsigned short POLICELIGHTPOLE;
#define POLICELIGHTPOLE__STATIC 4377
extern unsigned short FLOORBASKET;
#define FLOORBASKET__STATIC 4388
extern unsigned short PUKE;
#define PUKE__STATIC 4389
extern unsigned short DOORTILE23;
#define DOORTILE23__STATIC 4391
extern unsigned short TOPSECRET;
#define TOPSECRET__STATIC 4396
extern unsigned short SPEAKER;
#define SPEAKER__STATIC 4397
extern unsigned short TEDDYBEAR;
#define TEDDYBEAR__STATIC 4400
extern unsigned short ROBOTDOG;
#define ROBOTDOG__STATIC 4402
extern unsigned short ROBOTPIRATE;
#define ROBOTPIRATE__STATIC 4404
extern unsigned short ROBOTMOUSE;
#define ROBOTMOUSE__STATIC 4407
extern unsigned short MAIL;
#define MAIL__STATIC 4410
extern unsigned short MAILBAG;
#define MAILBAG__STATIC 4413
extern unsigned short HOTMEAT;
#define HOTMEAT__STATIC 4427
extern unsigned short COFFEEMUG;
#define COFFEEMUG__STATIC 4438
extern unsigned short DONUTS2;
#define DONUTS2__STATIC 4440
extern unsigned short TRIPODCAMERA;
#define TRIPODCAMERA__STATIC 4444
extern unsigned short METER;
#define METER__STATIC 4453
extern unsigned short DESKPHONE;
#define DESKPHONE__STATIC 4454
extern unsigned short GUMBALLMACHINE;
#define GUMBALLMACHINE__STATIC 4458
extern unsigned short GUMBALLMACHINEBROKE;
#define GUMBALLMACHINEBROKE__STATIC 4459
extern unsigned short PAPER;
#define PAPER__STATIC 4460
extern unsigned short MACE;
#define MACE__STATIC 4464
extern unsigned short GENERICPOLE2;
#define GENERICPOLE2__STATIC 4465
extern unsigned short XXXSTACY;
#define XXXSTACY__STATIC 4470
extern unsigned short WETFLOOR;
#define WETFLOOR__STATIC 4495
extern unsigned short BROOM;
#define BROOM__STATIC 4496
extern unsigned short MOP;
#define MOP__STATIC 4497
extern unsigned short LETTER;
#define LETTER__STATIC 4502
extern unsigned short PIRATE1A;
#define PIRATE1A__STATIC 4510
extern unsigned short PIRATE4A;
#define PIRATE4A__STATIC 4511
extern unsigned short PIRATE2A;
#define PIRATE2A__STATIC 4512
extern unsigned short PIRATE5A;
#define PIRATE5A__STATIC 4513
extern unsigned short PIRATE3A;
#define PIRATE3A__STATIC 4514
extern unsigned short PIRATE6A;
#define PIRATE6A__STATIC 4515
extern unsigned short PIRATEHALF;
#define PIRATEHALF__STATIC 4516
extern unsigned short CHESTOFGOLD;
#define CHESTOFGOLD__STATIC 4520
extern unsigned short SIDEBOLT1;
#define SIDEBOLT1__STATIC 4525
extern unsigned short FOODOBJECT1;
#define FOODOBJECT1__STATIC 4530
extern unsigned short FOODOBJECT2;
#define FOODOBJECT2__STATIC 4531
extern unsigned short FOODOBJECT3;
#define FOODOBJECT3__STATIC 4532
extern unsigned short FOODOBJECT4;
#define FOODOBJECT4__STATIC 4533
extern unsigned short FOODOBJECT5;
#define FOODOBJECT5__STATIC 4534
extern unsigned short FOODOBJECT6;
#define FOODOBJECT6__STATIC 4535
extern unsigned short FOODOBJECT7;
#define FOODOBJECT7__STATIC 4536
extern unsigned short FOODOBJECT8;
#define FOODOBJECT8__STATIC 4537
extern unsigned short FOODOBJECT9;
#define FOODOBJECT9__STATIC 4538
extern unsigned short FOODOBJECT10;
#define FOODOBJECT10__STATIC 4539
extern unsigned short FOODOBJECT11;
#define FOODOBJECT11__STATIC 4540
extern unsigned short FOODOBJECT12;
#define FOODOBJECT12__STATIC 4541
extern unsigned short FOODOBJECT13;
#define FOODOBJECT13__STATIC 4542
extern unsigned short FOODOBJECT14;
#define FOODOBJECT14__STATIC 4543
extern unsigned short FOODOBJECT15;
#define FOODOBJECT15__STATIC 4544
extern unsigned short FOODOBJECT16;
#define FOODOBJECT16__STATIC 4545
extern unsigned short FOODOBJECT17;
#define FOODOBJECT17__STATIC 4546
extern unsigned short FOODOBJECT18;
#define FOODOBJECT18__STATIC 4547
extern unsigned short FOODOBJECT19;
#define FOODOBJECT19__STATIC 4548
extern unsigned short FOODOBJECT20;
#define FOODOBJECT20__STATIC 4549
extern unsigned short HEADLAMP;
#define HEADLAMP__STATIC 4550
extern unsigned short TAMPON;
#define TAMPON__STATIC 4557
extern unsigned short SKINNEDCHICKEN;
#define SKINNEDCHICKEN__STATIC 4554
extern unsigned short FEATHEREDCHICKEN;
#define FEATHEREDCHICKEN__STATIC 4555
extern unsigned short ROBOTDOG2;
#define ROBOTDOG2__STATIC 4560
extern unsigned short JOLLYMEAL;
#define JOLLYMEAL__STATIC 4569
extern unsigned short DUKEBURGER;
#define DUKEBURGER__STATIC 4570
extern unsigned short SHOPPINGCART;
#define SHOPPINGCART__STATIC 4576
extern unsigned short CANWITHSOMETHING2;
#define CANWITHSOMETHING2__STATIC 4580
extern unsigned short CANWITHSOMETHING3;
#define CANWITHSOMETHING3__STATIC 4581
extern unsigned short CANWITHSOMETHING4;
#define CANWITHSOMETHING4__STATIC 4582
extern unsigned short SNAKEP;
#define SNAKEP__STATIC 4590
extern unsigned short DOLPHIN1;
#define DOLPHIN1__STATIC 4591
extern unsigned short DOLPHIN2;
#define DOLPHIN2__STATIC 4592
extern unsigned short NEWBEAST;
#define NEWBEAST__STATIC 4610
extern unsigned short NEWBEASTSTAYPUT;
#define NEWBEASTSTAYPUT__STATIC 4611
extern unsigned short NEWBEASTJUMP;
#define NEWBEASTJUMP__STATIC 4690
extern unsigned short NEWBEASTHANG;
#define NEWBEASTHANG__STATIC 4670
extern unsigned short NEWBEASTHANGDEAD;
#define NEWBEASTHANGDEAD__STATIC 4671
extern unsigned short BOSS4;
#define BOSS4__STATIC 4740
extern unsigned short BOSS4STAYPUT;
#define BOSS4STAYPUT__STATIC 4741
extern unsigned short FEM10;
#define FEM10__STATIC 4864
extern unsigned short TOUGHGAL;
#define TOUGHGAL__STATIC 4866
extern unsigned short MAN;
#define MAN__STATIC 4871
extern unsigned short MAN2;
#define MAN2__STATIC 4872
extern unsigned short WOMAN;
#define WOMAN__STATIC 4874
extern unsigned short PLEASEWAIT;
#define PLEASEWAIT__STATIC 4887
extern unsigned short NATURALLIGHTNING;
#define NATURALLIGHTNING__STATIC 4890
extern unsigned short WEATHERWARN;
#define WEATHERWARN__STATIC 4893
extern unsigned short DUKETAG;
#define DUKETAG__STATIC 4900
extern unsigned short SIGN1;
#define SIGN1__STATIC 4909
extern unsigned short SIGN2;
#define SIGN2__STATIC 4912
extern unsigned short JURYGUY;
#define JURYGUY__STATIC 4943
extern unsigned short RESERVEDSLOT1;
#define RESERVEDSLOT1__STATIC 6132
extern unsigned short RESERVEDSLOT2;
#define RESERVEDSLOT2__STATIC 6133
extern unsigned short RESERVEDSLOT3;
#define RESERVEDSLOT3__STATIC 6134
extern unsigned short RESERVEDSLOT4;
#define RESERVEDSLOT4__STATIC 6135
extern unsigned short RESERVEDSLOT5;
#define RESERVEDSLOT5__STATIC 6136
extern unsigned short RESERVEDSLOT6;
#define RESERVEDSLOT6__STATIC 6137
extern unsigned short RESERVEDSLOT7;
#define RESERVEDSLOT7__STATIC 6138
extern unsigned short RESERVEDSLOT8;
#define RESERVEDSLOT8__STATIC 6139
extern unsigned short RESERVEDSLOT9;
#define RESERVEDSLOT9__STATIC 6140
extern unsigned short RESERVEDSLOT10;
#define RESERVEDSLOT10__STATIC 6141
extern unsigned short RESERVEDSLOT11;
#define RESERVEDSLOT11__STATIC 6142
extern unsigned short RESERVEDSLOT12;
#define RESERVEDSLOT12__STATIC 6143
extern unsigned short dynamictostatic[MAXTILES];
