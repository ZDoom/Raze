#pragma once

#include "screenjob.h"
#include "constants.h"
#include "packet.h"
#include "types.h"

struct MapRecord;

BEGIN_DUKE_NS



// dumping ground for all external function prototypes to keep them out of the important headers.
// This list is not sorted in any way.

void lava_cleararrays();
void addjaildoor(int p1, int p2, int iht, int jlt, int p3, sectortype* h);
void addminecart(int p1, int p2, sectortype* i, int iht, int p3, sectortype* childsectnum);
void addtorch(sectortype* sect, int shade, int lotag);
void addlightning(sectortype* sect, int shade);
int addambient(int hitag, int lotag);

bool ceilingspace(sectortype* sectp);
bool floorspace(sectortype* sectp);

void movecyclers(void);
void movedummyplayers(void);
void resetlanepics(void);
void moveplayers();
void doanimations();
void tickstat(int stat, bool deleteinvalid = false);
void operaterespawns(int low);
void moveclouds(double interpfrac);
void movefta();

void clearcameras(player_struct* p);
void RANDOMSCRAP(DDukeActor* i);
void detonate(DDukeActor* i, int explosion);
void lotsofstuff(DDukeActor* s, int n, int spawntype);
void watersplash2(DDukeActor* i);
bool money(DDukeActor* i, int BLOODPOOL);
bool bloodpool(DDukeActor* i, bool puke);
void shell(DDukeActor* i, bool morecheck);
void glasspieces(DDukeActor* i);
void spawnguts(DDukeActor* origin, PClass* type, int count);

void handle_se00(DDukeActor* i);
void handle_se01(DDukeActor* i);
void handle_se14(DDukeActor* i, bool checkstat, int RPG, int JIBS6);
void handle_se30(DDukeActor* i, int JIBS6);
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
void handle_se19(DDukeActor* i, int BIGFORCE);
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
void handle_se35(DDukeActor* i, int SMALLSMOKE, int EXPLOSION2);
void handle_se128(DDukeActor* i);
void handle_se130(DDukeActor* i, int countmax, int EXPLOSION2);

void check_fta_sounds_d(DDukeActor* i);
void check_fta_sounds_r(DDukeActor* i);

int dodge(DDukeActor*);
void alterang(int ang, DDukeActor* actor, int g_p);
void fall_common(DDukeActor* actor, int g_p, int JIBS6, int DRONE, int BLOODPOOL, int SHOTSPARK1, int squished, int thud, int(*fallspecial)(DDukeActor*, int));
void checkavailweapon(player_struct* p);
void addammo(int weapon, player_struct* p, int amount);

int ssp(DDukeActor* i, unsigned int cliptype); //The set sprite function
void insertspriteq(DDukeActor *i);
int wakeup(DDukeActor* sn, int pn);


int timedexit(int snum);
void dokneeattack(int snum);
int endoflevel(int snum);
void playerisdead(int snum, int psectlotag, double fz, double cz);
void footprints(int snum);
int makepainsounds(int snum, int type);
void playerCrouch(int snum);
void playerJump(int snum, double fz, double cz);

void checklook(int snum, ESyncBits actions);
void playerCenterView(int snum);
void playerLookUp(int snum, ESyncBits actions);
void playerLookDown(int snum, ESyncBits actions);
void playerAimUp(int snum, ESyncBits actions);
void playerAimDown(int snum, ESyncBits actions);
void tracers(const DVector3& start, const DVector3& dest, int n);
DDukeActor* aim(DDukeActor* s, int aang);
void checkweapons(player_struct* const p);
int findotherplayer(int p, double* d);
void quickkill(player_struct* p);
int setpal(player_struct* p);
int madenoise(int playerNum);
int haskey(sectortype* sect, int snum);

void breakwall(int newpn, DDukeActor* spr, walltype* dawallnum);
int callsound(sectortype* sectnum,DDukeActor* snum, bool endstate = false);
double hitasprite(DDukeActor* snum,DDukeActor **hitSprite);
int findplayer(const DDukeActor* s, double* dist);

void operatejaildoors(int hitag);
void allignwarpelevators(void);
bool isablockdoor(int tileNum);
bool activatewarpelevators(DDukeActor* s, int w);
int check_activator_motion(int lotag);
void operateactivators(int l, player_struct* w);
void operateforcefields_common(DDukeActor* s, int low, const std::initializer_list<int>& tiles);
void operatemasterswitches(int lotag);
void operatesectors(sectortype* s, DDukeActor* i);
void hud_input(int playerNum);
int getanimationindex(int animtype, sectortype* animindex);
bool isanearoperator(int lotag);
bool isanunderoperator(int lotag);
int setanimation(sectortype* animsect, int animtype, walltype* animtarget, double thegoal, double thevel);
int setanimation(sectortype* animsect, int animtype, sectortype* animtarget, double thegoal, double thevel);
void dofurniture(walltype* wallNum, sectortype* sectnum, int playerNum);
void dotorch();
double hitawall(player_struct* pl, walltype** hitWall);
double hits(DDukeActor* snum);

DDukeActor* LocateTheLocator(int n, sectortype* sectnum);
void clearcamera(player_struct* ps);

void LoadActor(DDukeActor* i, int p, int x);
bool execute(DDukeActor* s, int p, double d);
void makeitfall(DDukeActor* s);
DAngle furthestangle(DDukeActor* snum, int angDiv);
void getglobalz(DDukeActor* s);
void OnEvent(int id, int pnum = -1, DDukeActor* snum = nullptr, int dist = -1);
void setFromSpawnRec(DDukeActor* act, SpawnRec* info);

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, int s_pn, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat);
DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat);
DDukeActor* SpawnActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat = -1);

void ceilingglass(DDukeActor* snum, sectortype* sectnum, int cnt);
void spriteglass(DDukeActor* snum, int cnt);
void lotsofcolourglass(DDukeActor* snum, walltype* wallNum, int cnt);
void lotsofglass(DDukeActor* snum, walltype* wal, int cnt);
void checkplayerhurt_d(player_struct* p, const Collision& coll);
void checkplayerhurt_r(player_struct* p, const Collision& coll);
DDukeActor* dospawnsprite(DDukeActor* actj, int pn);

void spriteinit_d(DDukeActor*);
void spriteinit_r(DDukeActor*);
DDukeActor* spawninit_d(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors);
DDukeActor* spawninit_r(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors);

void addspritetodelete(int spnum=0);
void checkavailinven(player_struct* p);
bool initspriteforspawn(DDukeActor* spn);
void spawninitdefault(DDukeActor* actj, DDukeActor* act);
void spawntransporter(DDukeActor* actj, DDukeActor* acti, bool beam);
int spawnbloodpoolpart1(DDukeActor* acti);
void initshell(DDukeActor* actj, DDukeActor* acti, bool isshell);
void spawneffector(DDukeActor* actor, TArray<DDukeActor*>* actors);
int startrts(int lumpNum, int localPlayer);

void pickrandomspot(int pn);
void premapcontroller(DDukeActor* ac);
void resetinventory(int pn);
void resetplayerstats(int pn);
void resetweapons(int pn);
void resetprestat(int snum, int g);
void prelevel_common(int g);
void cacheit_d();
void cacheit_r();

void FTA(int q, player_struct* p);
void OnMotorcycle(player_struct *pl);
void OffMotorcycle(player_struct *pl);
void OnBoat(player_struct *pl);
void OffBoat(player_struct *pl);

void cameratext(DDukeActor* i);
void dobonus(int bonusonly, const CompletionFunc& completion);

void drawweapon(double interpfrac);
void drawoverlays(double interpfrac);
void drawbackground(void);
void displayrooms(int32_t playerNum, double interpfrac, bool sceneonly);
void setgamepalette(int palid);
void resetmys();
void resettimevars();
bool setnextmap(bool checksecretexit);
void prelevel_d(int g, TArray<DDukeActor*>&);
void prelevel_r(int g, TArray<DDukeActor*>&);
void e4intro(const CompletionFunc& completion);
void exitlevel(MapRecord *next);
void enterlevel(MapRecord* mi, int gm);
void donewgame(MapRecord* map, int sk);
int playercolor2lookup(int color);
void PlayerColorChanged(void);
bool movementBlocked(player_struct *p);
void loadcons();
void DrawStatusBar();

void drawshadows(tspriteArray& tsprites, tspritetype* t, DDukeActor* h);

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
