#pragma once

#include "screenjob.h"
#include "constants.h"
#include "packet.h"
#include "types.h"
#include "g_mapinfo.h"

struct MapRecord;

BEGIN_DUKE_NS



// dumping ground for all external function prototypes to keep them out of the important headers.
// This list is not sorted in any way.

void animatewalls(void);
void lava_cleararrays();
void addjaildoor(int p1, int p2, int iht, int jlt, int p3, sectortype* h);
void addminecart(int p1, int p2, sectortype* i, int iht, int p3, sectortype* childsectnum);
void addthundersector(sectortype* sect);
void addtorch(sectortype* sect, int shade, int lotag);
void addlightning(sectortype* sect, int shade);
int addambient(int hitag, int lotag);

bool ceilingspace(sectortype* sectp);
bool floorspace(sectortype* sectp);

void movecyclers(void);
void movedummyplayers(void);
void resetlanepics(void);
void moveplayers();
void movefallers();
void doanimations();
void checkdive(DDukeActor* transporter, DDukeActor* transported);
void tickstat(int stat, bool deleteinvalid = false);
void operaterespawns(int low);
void moveclouds(double interpfrac);
void movefta();

void clearcameras(DDukePlayer* p);
void RANDOMSCRAP(DDukeActor* i);
void detonate(DDukeActor* i, PClassActor* explosion);
void hitradius(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
int movesprite_ex(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result);
void move(DDukeActor* actor, int pnum, double xvel);
void lotsofstuff(DDukeActor* s, int n, PClassActor* spawntype);
void watersplash2(DDukeActor* i);
bool money(DDukeActor* i, int BLOODPOOL);
bool bloodpool(DDukeActor* i, bool puke);
void shell(DDukeActor* i, bool morecheck);
void glasspieces(DDukeActor* i);
void spawnguts(DDukeActor* origin, PClass* type, int count);

void handle_se00(DDukeActor* i);
void handle_se01(DDukeActor* i);
void handle_se14(DDukeActor* i, bool checkstat, PClassActor* RPG);
void handle_se30(DDukeActor* i);
void handle_se02(DDukeActor* i);
void handle_se03(DDukeActor* i);
void handle_se04(DDukeActor* i);
void handle_se05(DDukeActor* i);
void handle_se08(DDukeActor* i, bool checkhitag1);
void handle_se10(DDukeActor* i, const int *);
void handle_se11(DDukeActor* i);
void handle_se12(DDukeActor* i, int planeonly = 0);
void handle_se13(DDukeActor* i);
void handle_se15(DDukeActor* i);
void handle_se16(DDukeActor* i);
void handle_se17(DDukeActor* i);
void handle_se18(DDukeActor* i, bool morecheck);
void handle_se19(DDukeActor* i);
void handle_se20(DDukeActor* i);
void handle_se21(DDukeActor* i);
void handle_se22(DDukeActor* i);
void handle_se24(DDukeActor* actor, bool scroll, double shift);
void handle_se25(DDukeActor* a, int snd1, int snd2);
void handle_se26(DDukeActor* i);
void handle_se27(DDukeActor* i);
void handle_se29(DDukeActor* actor);
void handle_se31(DDukeActor* a, bool choosedir);
void handle_se32(DDukeActor* i);
void handle_se35(DDukeActor* i);
void handle_se36(DDukeActor* i);
void handle_se128(DDukeActor* i);
void handle_se130(DDukeActor* i, int countmax);

int dodge(DDukeActor*);
void alterang(int ang, DDukeActor* actor, DDukePlayer* const p);
void checkavailweapon(DDukePlayer* p);
void addammo(int weapon, DDukePlayer* p, int amount);

int ssp(DDukeActor* i, unsigned int cliptype); //The set sprite function
void insertspriteq(DDukeActor *i);
int madenoise(DDukePlayer* const p);
int wakeup(DDukeActor* sn, DDukePlayer* const p);


int timedexit(int snum);
void dokneeattack(DDukePlayer* const p);
int endoflevel(int snum);
void playerisdead(DDukePlayer* const p, int psectlotag, double fz, double cz);
void footprints(DDukePlayer* const p);
int makepainsounds(DDukePlayer* const p, int type);
void playerCrouch(int snum);
void playerJump(int snum, double fz, double cz);

void checklook(int snum, ESyncBits actions);
void playerCenterView(int snum);
void playerLookUp(int snum, ESyncBits actions);
void playerLookDown(int snum, ESyncBits actions);
void playerAimUp(int snum, ESyncBits actions);
void playerAimDown(int snum, ESyncBits actions);
DDukeActor* aim(DDukeActor* s, int aang, bool force = true, bool* b = nullptr);
DDukeActor* aim_(DDukeActor* actor, DDukeActor* weapon, double aimangle, bool* b = nullptr);
void shoot(DDukeActor* actor, PClass* cls);
void checkweapons(DDukePlayer* const p);
int findotherplayer(int p, double* d);
void quickkill(DDukePlayer* p);
int setpal(DDukePlayer* p);
int haslock(sectortype* sect, int snum);
void purplelavacheck(DDukePlayer* p);
void addphealth(DDukePlayer* p, int amount, bool bigitem);
int playereat(DDukePlayer* p, int amount, bool bigitem);
void playerdrink(DDukePlayer* p, int amount);
int playeraddammo(DDukePlayer* p, int weaponindex, int amount);
int playeraddweapon(DDukePlayer* p, int weaponindex, int amount);
void playeraddinventory(DDukePlayer* p, DDukeActor* item, int type, int amount);
void actorsizeto(DDukeActor* actor, double x, double y);
void spawndebris(DDukeActor* g_ac, int dnum, int count);
int checkp(DDukeActor* self, DDukePlayer* p, int flags);
int playercheckinventory(DDukePlayer* p, DDukeActor* item, int type, int amount);
void playerstomp(DDukePlayer* p, DDukeActor* stomped);
void playerreset(DDukePlayer* p, DDukeActor* g_ac);
void wackplayer(DDukePlayer* p);
void actoroperate(DDukeActor* g_ac);
void playerkick(DDukePlayer* p, DDukeActor* g_ac);
void garybanjo(DDukeActor* g_ac);
int ifsquished(DDukeActor* i, int p);
void fakebubbaspawn(DDukeActor* actor, DDukePlayer* p);
void tearitup(sectortype* sect);
void destroyit(DDukeActor* actor);
void mamaspawn(DDukeActor* actor);
void forceplayerangle(DDukePlayer* snum);


bool checkhitceiling(sectortype* sectp);
void checkhitwall(DDukeActor* spr, walltype* wal, const DVector3& pos);
int callsound(sectortype* sectnum,DDukeActor* snum, bool endstate = false);
double hitasprite(DDukeActor* snum,DDukeActor **hitSprite);
int findplayer(const DDukeActor* s, double* dist);

void operatejaildoors(int hitag);
void allignwarpelevators(void);
bool isablockdoor(int tileNum);
bool activatewarpelevators(DDukeActor* s, int w);
int check_activator_motion(int lotag);
void operateactivators(int l, DDukePlayer* w);
void operateforcefields(DDukeActor* s, int low);
void operatemasterswitches(int lotag);
void operatesectors(sectortype* s, DDukeActor* i);
void hud_input(DDukePlayer* const p);
int getanimationindex(int animtype, sectortype* animindex);
bool isanearoperator(int lotag);
bool isanunderoperator(int lotag);
int setanimation(sectortype* animsect, int animtype, walltype* animtarget, double thegoal, double thevel);
int setanimation(sectortype* animsect, int animtype, sectortype* animtarget, double thegoal, double thevel);
void dofurniture(walltype* wallNum, sectortype* sectnum, int playerNum);
void dotorch();
double hitawall(DDukePlayer* pl, walltype** hitWall);
double hits(DDukeActor* snum);

DDukeActor* LocateTheLocator(int n, sectortype* sectnum);
void clearcamera(DDukePlayer* ps);

void LoadActor(DDukeActor* i, int p, int x);
bool execute(DDukeActor* s, int p, double d, int* killit_flag = nullptr);
void makeitfall(DDukeActor* s);
DAngle furthestangle(DDukeActor* snum, int angDiv);
void getglobalz(DDukeActor* s);
void OnEvent(int id, int pnum = -1, DDukeActor* snum = nullptr, int dist = -1);
void setFromSpawnRec(DDukeActor* act, SpawnRec* info);

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat);
DDukeActor* SpawnActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat = -1);

void ceilingglass(DDukeActor* snum, sectortype* sectnum, int cnt);
void spriteglass(DDukeActor* snum, int cnt);
void lotsofcolourglass(DDukeActor* snum, walltype* wallNum, int cnt);
void lotsofglass(DDukeActor* snum, walltype* wal, int cnt);
void checkplayerhurt_d(DDukePlayer* p, const Collision& coll);
void checkplayerhurt_r(DDukePlayer* p, const Collision& coll);
DDukeActor* dospawnsprite(DDukeActor* actj, int pn);

void spriteinit(DDukeActor*, TArray<DDukeActor*>& actors);
DDukeActor* spawninit(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors);

void checkavailinven(DDukePlayer* p);
bool initspriteforspawn(DDukeActor* spn);
void initshell(DDukeActor* actj, DDukeActor* acti, bool isshell);
void spawneffector(DDukeActor* actor, TArray<DDukeActor*>* actors);
int startrts(int lumpNum, int localPlayer);

void pickrandomspot(int pn);
void premapcontroller(DDukeActor* ac);
void resetinventory(DDukePlayer* pn);
void resetweapons(DDukePlayer* pn);
void resetprestat(int snum, int g);
void prelevel_common(int g);
void cacheit();

void FTA(int q, DDukePlayer* p);
void OnMotorcycle(DDukePlayer *pl);
void OffMotorcycle(DDukePlayer *pl);
void OnBoat(DDukePlayer *pl);
void OffBoat(DDukePlayer *pl);

void cameratext(DDukeActor* i);
void dobonus(int bonusonly, const CompletionFunc& completion);

void drawweapon(double interpfrac);
void drawoverlays(double interpfrac);
void drawbackground(void);
void displayrooms(int32_t playerNum, double interpfrac, bool sceneonly);
void setgamepalette(int palid);
void resetmys();
void resettimevars();
int setnextmap(bool checksecretexit);
void prelevel_d(int g, TArray<DDukeActor*>&);
void prelevel_r(int g, TArray<DDukeActor*>&);
void e4intro(const CompletionFunc& completion);
void exitlevel(MapRecord *next);
void enterlevel(MapRecord* mi, int gm);
void donewgame(MapRecord* map, int sk);
int playercolor2lookup(int color);
void PlayerColorChanged(void);
bool movementBlocked(DDukePlayer *p);
void underwater(int snum, ESyncBits actions, double floorz, double ceilingz);
void loadcons();
void DrawStatusBar();
void thunder(void);
bool checkhitswitch(int snum, walltype* wwal, DDukeActor* act);

void drawshadows(tspriteArray& tsprites, tspritetype* t, DDukeActor* h);
void applyanimations(tspritetype* t, DDukeActor* h, const DVector2& viewVec, DAngle viewang);

int LookupAction(PClass* self, FName name);
int LookupMove(PClass* self, FName name);
int LookupAI(PClass* self, FName name);


inline int32_t krand(void)
{
	randomseed = (randomseed * 27584621) + 1;
	return ((uint32_t)randomseed) >> 16;
}

inline double krandf(double span)
{
	return (krand() & 0x7fff) * span / 32767;
}

inline double zrand(double spread)
{
	int r = krand() % FloatToFixed<8>(spread);
	return FixedToFloat<8>(r);
}

END_DUKE_NS
