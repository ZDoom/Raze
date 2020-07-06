#pragma once

// shared functions
void movecyclers(void);
void movedummyplayers(void);
void resetlanepics(void);
void moveplayers();
void doanimations();
void movefx();

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

