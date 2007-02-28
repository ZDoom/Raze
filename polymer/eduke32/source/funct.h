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

#ifndef __funct_h__
#define __funct_h__

struct player_struct;     // JBF: duke3d.h defines this later

extern void sendscore(const char *s);
extern void SoundStartup(void);
extern void SoundShutdown(void);
extern void MusicStartup(void);
extern void MusicShutdown(void);
extern void intomenusounds(void);
extern void playmusic(const char *fn);
extern int loadsound(unsigned num);
extern int xyzsound(int num,int i,long x,long y,long z);
extern void sound(int num);
extern int spritesound(unsigned int num,int i);
extern void stopsound(int num);
extern void stopenvsound(int num,int i);
extern void pan3dsound(void);
extern void testcallback(unsigned long num);
extern void clearsoundlocks(void);
extern int callsound(int sn,int whatsprite);
extern int check_activator_motion(int lotag);
extern int isadoorwall(int dapic);
extern int isanunderoperator(int lotag);
extern int isanearoperator(int lotag);
extern int checkcursectnums(int sect);
extern long ldist(spritetype *s1,spritetype *s2);
extern long dist(spritetype *s1,spritetype *s2);
extern int findplayer(spritetype *s,long *d);
extern int findotherplayer(int p,long *d);
extern void doanimations(void);
extern int getanimationgoal(long *animptr);
extern int setanimation(int animsect,long *animptr,long thegoal,long thevel);
extern void animatecamsprite(void);
extern void animatewalls(void);
extern int activatewarpelevators(int s,int d);
extern void operatesectors(int sn,int ii);
extern void operaterespawns(int low);
extern void operateactivators(int low,int snum);
extern void operatemasterswitches(int low);
extern void operateforcefields(int s,int low);
extern int checkhitswitch(int snum,long w,int switchtype);
extern void activatebysector(int sect,int j);
extern void checkhitwall(int spr,int dawallnum,long x,long y,long z,int atwith);
extern void checkplayerhurt(struct player_struct *p,int j);
extern int checkhitceiling(int sn);
extern void checkhitsprite(int i,int sn);
extern void allignwarpelevators(void);
extern void sharedkeys(int snum);
extern void checksectors(int snum);
extern int32 RTS_AddFile(const char *filename);
extern void RTS_Init(const char *filename);
extern int32 RTS_NumSounds(void);
extern int32 RTS_SoundLength(int32 lump);
extern const char *RTS_GetSoundName(int32 i);
extern void RTS_ReadLump(int32 lump,void *dest);
extern void *RTS_GetSound(int32 lump);
extern void docacheit(void);
extern void xyzmirror(int i,int wn);
extern void vscrn(void);
extern void pickrandomspot(int snum);
extern void resetweapons(int snum);
extern void resetinventory(int snum);
extern void newgame(int vn,int ln,int sk);
extern void resettimevars(void);
extern void waitforeverybody(void);
extern void cacheit(void);
extern void clearfifo(void);
extern void resetmys(void);
extern int  enterlevel(int g);
extern void backtomenu(void);
extern void setpal(struct player_struct *p);
extern void quickkill(struct player_struct *p);
extern long hits(int i);
extern long hitasprite(int i,short *hitsp);
extern int shoot(int i,int atwith);
extern void displaymasks(int snum);
extern void displayweapon(int snum);
extern void getinput(int snum);
extern void checkweapons(struct player_struct *p);
extern void processinput(int snum);
extern void cmenu(int cm);
extern void savetemp(const char *fn,long daptr,long dasiz);
// extern int loadpheader(char spot,int32 *vn,int32 *ln,int32 *psk,int32 *numplr);
extern int loadplayer(int spot);
extern int saveplayer(int spot);
extern inline int menutext(int x,int y,int s,int p,const char *t);
extern void menus(void);
extern void palto(int r,int g,int b,long e);
extern void playanm(const char *fn,char);
extern int getincangle(int a,int na);
extern void getglobalz(int iActor);
extern void makeitfall(int iActor);
extern void loadefs(const char *fn);
extern int furthestangle(int iActor,int angs);
extern void execute(int iActor,int iPlayer,long lDist);
extern void overwritesprite(long thex,long they,int tilenum,int shade,int stat,int dapalnum);
extern inline int gametext(int x,int y,const char *t,int s,int dabits);
extern inline int gametextpal(int x,int y,const char *t,int s,int p);
extern inline int minitext(int x,int y,const char *t,int p,int sb);
extern void gamenumber(long x,long y,long n,int s);
extern void Shutdown(void);
extern void getpackets(void);
extern void check_fta_sounds(int i);
extern inline int inventory(spritetype *s);
extern inline int badguy(spritetype *s);
extern int badguypic(int pn);
extern void myos(long x,long y,int tilenum,int shade,int orientation);
extern void myospal(long x,long y,int tilenum,int shade,int orientation,int p);
extern void displayfragbar(void);
extern void FTA(int q,struct player_struct *p);
extern void gameexit(const char *t);
extern inline int strget(int x,int y,char *t,int dalen,int c);
extern void displayrest(long smoothratio);
extern void updatesectorz(long x,long y,long z,short *sectnum);
extern void drawbackground(void);
extern void displayrooms(int snum,long smoothratio);
extern int EGS(int whatsect,long s_x,long s_y,long s_z,int s_pn,int s_s,int s_xr,int s_yr,int s_a,int s_ve,long s_zv,int s_ow,int s_ss);
extern int wallswitchcheck(int i);
extern int spawn(int j,int pn);
extern void animatesprites(long x,long y,int a,long smoothratio);
extern int main(int argc,char **argv);
extern void opendemowrite(void);
extern void closedemowrite(void);
extern void dobonus(int bonusonly);
extern void lotsofglass(int i,int wallnum,int n);
extern void spriteglass(int i,int n);
extern void ceilingglass(int i,int sectnum,int n);
extern void lotsofcolourglass(int i,int wallnum,int n);
extern long GetTime(void);
extern void CONFIG_GetSetupFilename(void);
extern int32 CONFIG_FunctionNameToNum(char *func);
extern char *CONFIG_FunctionNumToName(int32 func);
extern int32 CONFIG_AnalogNameToNum(char *func);
extern char *CONFIG_AnalogNumToName(int32 func);
extern void CONFIG_SetDefaults(void);
extern void CONFIG_ReadKeys(void);
extern void readsavenames(void);
extern int32 CONFIG_ReadSetup(void);
extern void CONFIG_WriteSetup(void);
extern void updateinterpolations(void);
extern void setinterpolation(long *posptr);
extern void stopinterpolation(long *posptr);
extern void dointerpolations(long smoothratio);
extern void restoreinterpolations(void);
extern long ceilingspace(int sectnum);
extern long floorspace(int sectnum);
extern void addammo(int weapon,struct player_struct *p,int amount);
extern void addweaponnoswitch(struct player_struct *p,int weapon);
extern void addweapon(struct player_struct *p,int weapon);
extern void checkavailinven(struct player_struct *p);
extern void checkavailweapon(struct player_struct *p);
extern void hitradius(int i,long r,long hp1,long hp2,long hp3,long hp4);
extern int movesprite(int spritenum,long xchange,long ychange,long zchange,unsigned long cliptype);
extern int ssp(int i,unsigned long cliptype);
extern void insertspriteq(int i);
extern void lotsofmoney(spritetype *s,int n);
extern void lotsofmail(spritetype *s, int n);
extern void lotsofpaper(spritetype *s, int n);
extern void guts(spritetype *s,int gtype,int n,int p);
extern void setsectinterpolate(int i);
extern void clearsectinterpolate(int i);
extern int ifhitsectors(int sectnum);
extern int ifhitbyweapon(int sn);
extern void moveobjects(void);
extern void movecyclers(void);
extern void movedummyplayers(void);

// game.c
extern inline void setstatusbarscale(long sc);

extern void setgamepalette(struct player_struct *player, char *pal, int set);
extern void fadepal(int r, int g, int b, int start, int end, int step);

extern inline int minitextshade(int x,int y,const char *t,int s,int p,int sb);
extern inline int gametext_(int small, int starttile, int x,int y,const char *t,int s,int p,int orientation,long x1, long y1, long x2, long y2);
extern void txdigitalnumber(int starttile, long x,long y,long n,int s,int pal,int cs,long x1, long y1, long x2, long y2);
extern void myosx(long x,long y,int tilenum,int shade,int orientation);
extern void myospalx(long x,long y,int tilenum,int shade,int orientation,int p);
extern void ResetGameVars(void);
extern void ResetActorGameVars(int iActor);

extern void setupdynamictostatic();
extern void processnames(const char *szLabel, long lValue);

extern void LoadActor(long sActor);

extern long GetGameVar(const char *szGameLabel, long lDefault, int iActor, int iPlayer);
extern void DumpGameVars(FILE *fp);
// extern void AddLog(const char *psz, ...);

extern void ResetSystemDefaults(void);
extern void InitGameVarPointers(void);
extern void InitGameVars(void);
extern void SaveGameVars(FILE *fil);
extern int ReadGameVars(long fil);

extern long GetGameVarID(int id, int iActor, int iPlayer);
extern void SetGameVarID(int id, long lValue, int iActor, int iPlayer);
extern int AddGameVar(const char *pszLabel, long lValue, unsigned long dwFlags);
extern void ReportError(int iError);

extern void onvideomodechange(int newmode);

extern void OnEvent(int iEventID, int sActor, int sPlayer, long lDist);

extern int isspritemakingsound(int i, int num);
extern int issoundplaying(int i, int num);
extern void stopspritesound(int num, int i);
extern void updateplayer(void);
extern void sendboardname(void);
extern void sendquit(void);

extern void adduserquote(const char *daquote);
extern const char *stripcolorcodes(const char *t);
extern void mpchangemap(int volume, int level);

extern inline int checkspriteflags(int iActor, int iType);
extern inline int checkspriteflagsp(int iPicnum, int iType);

extern int getteampal(int team);

#endif // __funct_h__
