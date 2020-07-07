#pragma once

#include "screenjob.h"
#include "constants.h"

struct MapRecord;

BEGIN_DUKE_NS

// dumping ground for all external function prototypes to keep them out of the important headers.
// This list is not sorted in any way.

void lava_cleararrays();
void addjaildoor(int p1, int p2, int iht, int jlt, int p3, int h);
void addminecart(int p1, int p2, int i, int iht, int p3, int childsectnum);
void addtorch(int i);
void addlightning(int i);

void movecyclers(void);
void movedummyplayers(void);
void resetlanepics(void);
void moveplayers();
void doanimations();
void movefx();
void moveclouds(void);

void RANDOMSCRAP(spritetype* s, int i);
void ms(short i);
void movecrane(int i, int crane);
void movefountain(int i, int fountain);
void moveflammable(int i, int tire, int box, int pool);
void detonate(int i, int explosion);
void movemasterswitch(int i, int spectype1, int spectype2);
void movetrash(int i);
void movewaterdrip(int i, int drip);
void movedoorshock(int i);
void movetouchplate(int i, int plate);
void movecanwithsomething(int i);
void bounce(int i);
void movetongue(int i, int tongue, int jaw);
void moveooz(int i, int seenine, int seeninedead, int ooz, int explosion);
void lotsofstuff(spritetype* s, short n, int spawntype);
bool respawnmarker(int i, int yellow, int green);
bool rat(int i, bool makesound);
bool queball(int i, int pocket, int queball, int stripeball);
void forcesphere(int i, int forcesphere);
void recon(int i, int explosion, int firelaser, int attacksnd, int painsnd, int roamsnd, int shift, int (*getspawn)(int i));
void ooz(int i);
void reactor(int i, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT);
void camera(int i);
void forcesphere(int i);
void watersplash2(int i);
void frameeffect1(int i);
bool money(int i, int BLOODPOOL);
bool jibs(int i, int JIBS6, bool timeout, bool callsetsprite, bool floorcheck, bool zcheck1, bool zcheck2);
bool bloodpool(int i, bool puke, int TIRE);
void shell(int i, bool morecheck);
void glasspieces(int i);
void scrap(int i, int SCRAP1, int SCRAP6);

void handle_se00(int i, int LASERLINE);
void handle_se01(int i);
void handle_se14(int i, bool checkstat, int RPG, int JIBS6);
void handle_se30(int i, int JIBS6);
void handle_se02(int i);
void handle_se03(int i);
void handle_se04(int i);
void handle_se05(int i, int FIRELASER);
void handle_se08(int i, bool checkhitag1);
void handle_se10(int i, const int *);
void handle_se11(int i);
void handle_se12(int i, int planeonly = 0);
void handle_se13(int i);
void handle_se15(int i);
void handle_se16(int i, int REACTOR, int REACTOR2);
void handle_se17(int i);
void handle_se18(int i, bool morecheck);
void handle_se19(int i, int BIGFORCE);
void handle_se20(int i);
void handle_se21(int i);
void handle_se22(int i);
void handle_se26(int i);
void handle_se27(int i);
void handle_se32(int i);
void handle_se35(int i, int SMALLSMOKE, int EXPLOSION2);
void handle_se128(int i);
void handle_se130(int i, int countmax, int EXPLOSION2);

void respawn_rrra(int i, int j);

int dodge(spritetype*);
void alterang(int a, int g_i, int g_p);
void fall_common(int g_i, int g_p, int JIBS6, int DRONE, int BLOODPOOL, int SHOTSPARK1, int squished, int thud, int(*fallspecial)(int, int), void (*falladjustz)(spritetype*));
void checkavailweapon(struct player_struct* p);
void deletesprite(int num);
void addammo(int weapon, struct player_struct* p, int amount);

int ssp(int i, unsigned int cliptype); //The set sprite function
void insertspriteq(int i);
int wakeup(int sn, int pn);


int timedexit(int snum);
void dokneeattack(int snum, int pi, const std::initializer_list<int>& respawnlist);
int endoflevel(int snum);
void playerisdead(int snum, int psectlotag, int fz, int cz);
void footprints(int snum);
int makepainsounds(int snum, int type);
void playerCrouch(int snum);
void playerJump(int snum, int fz, int cz);
void playerLookLeft(int snum);
void playerLookRight(int snum);
void playerCenterView(int snum);
void playerLookUp(int snum, ESyncBits sb_snum);
void playerLookDown(int snum, ESyncBits sb_snum);
void playerAimUp(int snum, ESyncBits sb_snum);
void playerAimDown(int snum, ESyncBits sb_snum);
bool view(struct player_struct* pp, int* vx, int* vy, int* vz, short* vsectnum, int ang, int horiz);
void tracers(int x1, int y1, int z1, int x2, int y2, int z2, int n);
int hits(int i);
int hitasprite(int i, short* hitsp);
int aim(spritetype* s, int aang);
void checkweapons(struct player_struct* const p);
int findotherplayer(int p, int* d);
void quickkill(struct player_struct* p);
void setpal(struct player_struct* p);
int madenoise(int playerNum);
int haskey(int sect, int snum);

void breakwall(short newpn, short spr, short dawallnum);
void callsound2(int soundNum, int playerNum);
int callsound(int sectnum,int snum);
int hitasprite(int snum,short *hitSprite);
int findplayer(const spritetype* s, int* dist);
void operatejaildoors(int hitag);
void allignwarpelevators(void);
bool isablockdoor(int tileNum);
bool activatewarpelevators(int s, int w);
int check_activator_motion(int lotag);
void operateactivators(int l, int w);
void operateforcefields_common(int s, int low, const std::initializer_list<int>& tiles);
void operatemasterswitches(int lotag);
void operatesectors(int s, int i);
void hud_input(int playerNum);
int getanimationgoal(const int* animPtr);
bool isanearoperator(int lotag);
bool isanunderoperator(int lotag);
int setanimation(short animsect, int* animptr, int thegoal, int thevel);
void dofurniture(int wallNum, int sectnum, int playerNum);
void dotorch();
int hitawall(struct player_struct* pl, int* hitWall);
int hits(int snum);

void   clearsectinterpolate(int sprnum);
void   setsectinterpolate(int sprnum);
int LocateTheLocator(int const tag, int const sectnum);
void clearcamera(player_struct* ps);

void showtwoscreens(CompletionFunc func);
void doorders(CompletionFunc func);

void execute(int s, int p, int d);
void makeitfall(int s);
int furthestangle(int snum, int angDiv);
void getglobalz(int s);
int getincangle(int c, int n);
void OnEvent(int id, int pnum = -1, int snum = -1, int dist = -1);

short EGS(short whatsect, int s_x, int s_y, int s_z, short s_pn, signed char s_s, signed char s_xr, signed char s_yr, short s_a, short s_ve, int s_zv, short s_ow, signed char s_ss);
void ceilingglass(int snum, int sectnum, int cnt);
void spriteglass(int snum, int cnt);
void lotsofcolourglass(int snum, int wallNum, int cnt);
void lotsofglass(int snum, int wallnum, int cnt);

void addspritetodelete(int spnum);
void checkavailinven(struct player_struct* p);
int initspriteforspawn(int j, int pn, const std::initializer_list<int> &excludes);
void spawninitdefault(int j, int i);
void spawntransporter(int j, int i, bool beam);
int spawnbloodpoolpart1(int j, int i);
void initfootprint(int j, int i);
void initshell(int j, int i, bool isshell);
void initcrane(int j, int i, int CRANEPOLE);
void initwaterdrip(int j, int i);
int initreactor(int j, int i, bool isrecon);
void spawneffector(int i);
void gameexitfrommenu();

void pickrandomspot(int pn);
void resetinventory(int pn);
void resetplayerstats(int pn);
void resetweapons(int pn);
void resetprestat(int snum, int g);
void clearfifo(void);
void setmapfog(int fogtype);
void prelevel_common(int g);
void cacheit_d();
void cacheit_r();

void FTA(int q, struct player_struct* p);
void OnMotorcycle(player_struct *pl, int snum);
void OffMotorcycle(player_struct *pl);
void OnBoat(player_struct *pl, int snum);
void OffBoat(player_struct *pl);

void drawstatusbar_d(int snum);
void drawstatusbar_r(int snum);
void drawoverheadmap(int cposx, int cposy, int czoom, int cang);
void cameratext(int i);
void dobonus(int bonusonly);
void dobonus_d(bool bonusonly, CompletionFunc completion);
void dobonus_r(bool bonusonly, CompletionFunc completion);

void displayrest(int32_t smoothratio);
void drawbackground(void);
void displayrooms(int32_t playerNum, int32_t smoothratio);
void setgamepalette(int palid);
void resetmys();
void resettimevars();
bool setnextmap(bool checksecretexit);
void prelevel_d(int g);
void prelevel_r(int g);
void e4intro(CompletionFunc completion);
void clearfrags(void);
int exitlevel();
int enterlevel(MapRecord* mi, int gm);
void newgame(MapRecord* mi, int sk);

END_DUKE_NS
