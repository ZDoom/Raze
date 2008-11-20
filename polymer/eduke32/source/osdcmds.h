#ifndef __osdcmds_h__
#define __osdcmds_h__

struct osdcmd_cheatsinfo {
	int cheatnum;	// -1 = none, else = see DoCheats()
	int volume,level;
};

extern struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int registerosdcommands(void);

extern float r_ambientlight,r_ambientlightrecip;

enum cvartypes
{
    CVAR_INT,
    CVAR_UNSIGNEDINT,
    CVAR_BOOL,
    CVAR_STRING,
    CVAR_NOMULTI = 128,
    CVAR_MULTI = 256,
    CVAR_NOSAVE = 512
};

typedef struct
{
    char *name;
    char *helpstr;
    void *var;
    int type;       // 0 = integer, 1 = unsigned integer, 2 = boolean, 3 = string, |128 = not in multiplayer, |256 = update multi
    int extra;      // for string, is the length
    int min;
    int max;
} cvarmappings;

extern cvarmappings cvar[];

#endif	// __osdcmds_h__

