//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

#ifndef __funct_h__
#define __funct_h__

struct player_struct;     // JBF: duke3d.h defines this later

extern void sendscore(char *s);
extern void SoundStartup(void);
extern void SoundShutdown(void);
extern void MusicStartup(void);
extern void MusicShutdown(void);
extern void intomenusounds(void);
extern void playmusic(char *fn);
extern char loadsound(unsigned short num);
extern int xyzsound(short num,short i,long x,long y,long z);
extern void sound(short num);
extern int spritesound(unsigned short num,short i);
extern void stopsound(short num);
extern void stopenvsound(short num,short i);
extern void pan3dsound(void);
extern void testcallback(unsigned long num);
extern void clearsoundlocks(void);
extern short callsound(short sn,short whatsprite);
extern short check_activator_motion(short lotag);
extern char isadoorwall(short dapic);
extern char isanunderoperator(short lotag);
extern char isanearoperator(short lotag);
extern short checkcursectnums(short sect);
extern long ldist(spritetype *s1,spritetype *s2);
extern long dist(spritetype *s1,spritetype *s2);
extern short findplayer(spritetype *s,long *d);
extern short findotherplayer(short p,long *d);
extern void doanimations(void);
extern int getanimationgoal(long *animptr);
extern int setanimation(short animsect,long *animptr,long thegoal,long thevel);
extern void animatecamsprite(void);
extern void animatewalls(void);
extern char activatewarpelevators(short s,short d);
extern void operatesectors(short sn,short ii);
extern void operaterespawns(short low);
extern void operateactivators(short low,short snum);
extern void operatemasterswitches(short low);
extern void operateforcefields(short s,short low);
extern char checkhitswitch(short snum,long w,char switchtype);
extern void activatebysector(short sect,short j);
extern void checkhitwall(short spr,short dawallnum,long x,long y,long z,short atwith);
extern void checkplayerhurt(struct player_struct *p,short j);
extern char checkhitceiling(short sn);
extern void checkhitsprite(short i,short sn);
extern void allignwarpelevators(void);
extern void cheatkeys(short snum);
extern void checksectors(short snum);
extern int32 RTS_AddFile(char *filename);
extern void RTS_Init(char *filename);
extern int32 RTS_NumSounds(void);
extern int32 RTS_SoundLength(int32 lump);
extern char *RTS_GetSoundName(int32 i);
extern void RTS_ReadLump(int32 lump,void *dest);
extern void *RTS_GetSound(int32 lump);
extern void docacheit(void);
extern void xyzmirror(short i,short wn);
extern void vscrn(void);
extern void pickrandomspot(short snum);
extern void resetweapons(short snum);
extern void resetinventory(short snum);
extern void newgame(char vn,char ln,char sk);
extern void resettimevars(void);
extern void waitforeverybody(void);
extern void cacheit(void);
extern void clearfifo(void);
extern void resetmys(void);
extern int  enterlevel(char g);
extern void backtomenu(void);
extern void setpal(struct player_struct *p);
extern void quickkill(struct player_struct *p);
extern long hits(short i);
extern long hitasprite(short i,short *hitsp);
extern short shoot(short i,short atwith);
extern void displaymasks(short snum);
extern void displayweapon(short snum);
extern void getinput(short snum);
extern void checkweapons(struct player_struct *p);
extern void processinput(short snum);
extern void cmenu(short cm);
extern void savetemp(char *fn,long daptr,long dasiz);
// extern int loadpheader(char spot,int32 *vn,int32 *ln,int32 *psk,int32 *numplr);
extern int loadplayer(signed char spot);
extern int saveplayer(signed char spot);
extern int menutext(int x,int y,short s,short p,char *t);
extern int getfilenames(char *path, char kind[]);
extern void menus(void);
extern void palto(char r,char g,char b,long e);
extern void playanm(char *fn,char);
extern short getincangle(short a,short na);
extern void getglobalz(short sActor);
extern void makeitfall(short sActor);
extern void loadefs(char *fn);
extern short furthestangle(short sActor,short angs);
extern void execute(const short *sActor,const short *sPlayer,const long *lDist);
extern void overwritesprite(long thex,long they,short tilenum,signed char shade,char stat,char dapalnum);
extern int gametext(int x,int y,char *t,char s,short dabits);
extern int gametextpal(int x,int y,char *t,char s,char p);
extern int minitext(int x,int y,char *t,char p,short sb);
extern void gamenumber(long x,long y,long n,char s);
extern void Shutdown(void);
extern void getpackets(void);
extern void check_fta_sounds(short i);
extern inline short inventory(spritetype *s);
extern short badguy(spritetype *s);
extern short badguypic(short pn);
extern void myos(long x,long y,short tilenum,signed char shade,char orientation);
extern void myospal(long x,long y,short tilenum,signed char shade,char orientation,char p);
extern void displayfragbar(void);
extern void FTA(short q,struct player_struct *p);
extern void gameexit(char *t);
extern short strget(short x,short y,char *t,short dalen,short c);
extern void displayrest(long smoothratio);
extern void updatesectorz(long x,long y,long z,short *sectnum);
extern void drawbackground(void);
extern void displayrooms(short snum,long smoothratio);
extern short EGS(short whatsect,long s_x,long s_y,long s_z,short s_pn,signed char s_s,signed char s_xr,signed char s_yr,short s_a,short s_ve,long s_zv,short s_ow,signed char s_ss);
extern char wallswitchcheck(short i);
extern short spawn(short j,short pn);
extern void animatesprites(long x,long y,short a,long smoothratio);
extern void printstr(short x,short y,char string[],char attribute);
extern void Logo(void);
extern void loadtmb(void);
extern void compilecons(void);
extern int main(int argc,char **argv);
extern void opendemowrite(void);
extern void closedemowrite(void);
extern void dobonus(char bonusonly);
extern void lotsofglass(short i,short wallnum,short n);
extern void spriteglass(short i,short n);
extern void ceilingglass(short i,short sectnum,short n);
extern void lotsofcolourglass(short i,short wallnum,short n);
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
extern long ceilingspace(short sectnum);
extern long floorspace(short sectnum);
extern void addammo(short weapon,struct player_struct *p,short amount);
extern void addweaponnoswitch(struct player_struct *p,short weapon);
extern void addweapon(struct player_struct *p,short weapon);
extern void checkavailinven(struct player_struct *p);
extern void checkavailweapon(struct player_struct *p);
extern void hitradius(short i,long r,long hp1,long hp2,long hp3,long hp4);
extern int movesprite(short spritenum,long xchange,long ychange,long zchange,unsigned long cliptype);
extern short ssp(short i,unsigned long cliptype);
extern void insertspriteq(short i);
extern void lotsofmoney(spritetype *s,short n);
extern void lotsofmail(spritetype *s, short n);
extern void lotsofpaper(spritetype *s, short n);
extern void guts(spritetype *s,short gtype,short n,short p);
extern void setsectinterpolate(short i);
extern void clearsectinterpolate(short i);
extern short ifhitsectors(short sectnum);
extern short ifhitbyweapon(short sn);
extern void moveobjects(void);
extern void movecyclers(void);
extern void movedummyplayers(void);

// game.c
extern void setstatusbarscale(long sc);

extern void setgamepalette(struct player_struct *player, char *pal, int set);
extern void fadepal(int r, int g, int b, int start, int end, int step);

extern int minitextshade(int x,int y,char *t,char s,char p,short sb);
extern int gametext_(int small, int starttile, int x,int y,char *t,char s,char p,short orientation,long x1, long y1, long x2, long y2);
extern void txdigitalnumber(short starttile, long x,long y,long n,char s,char pal,char cs,long x1, long y1, long x2, long y2);
extern void myosx(long x,long y,short tilenum,signed char shade,char orientation);
extern void myospalx(long x,long y,short tilenum,signed char shade,char orientation,char p);
extern void ResetGameVars(void);
extern void ResetActorGameVars(short sActor);

extern void setupdynamictostatic();
extern void processnames(char *szLabel, long lValue);

extern void LoadActor(long sActor);

extern long GetGameVar(char *szGameLabel, long lDefault, short sActor, short sPlayer);
extern void DumpGameVars(FILE *fp);
extern void AddLog(char *psz);

extern void ResetSystemDefaults(void);
extern void InitGameVarPointers(void);
extern void InitGameVars(void);
extern void SaveGameVars(FILE *fil);
extern int ReadGameVars(long fil);

extern long GetGameVarID(int id, short sActor, short sPlayer);
extern void SetGameVarID(int id, long lValue, short sActor, short sPlayer);
extern char AddGameVar(char *pszLabel, long lValue, unsigned long dwFlags);
extern void ReportError(int iError);

extern void onvideomodechange(int newmode);

extern void OnEvent(int iEventID, short sActor,short sPlayer,long lDist);

extern int isspritemakingsound(short i, int num);
extern int issoundplaying(short i, int num);
extern void stopspritesound(short num, short i);
extern void updateplayer(void);
extern void sendboardname(void);
extern void sendquit(void);

extern void adduserquote(char *daquote);
extern char *strip_color_codes(char *t);

#endif // __funct_h__
