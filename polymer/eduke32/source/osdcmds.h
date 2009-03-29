#ifndef __osdcmds_h__
#define __osdcmds_h__

struct osdcmd_cheatsinfo {
	int32_t cheatnum;	// -1 = none, else = see DoCheats()
	int32_t volume,level;
};

extern struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int32_t registerosdcommands(void);

extern float r_ambientlight,r_ambientlightrecip;

enum cvartypes
{
    CVAR_FLOAT,
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
    int32_t type;       // 0 = integer, 1 = unsigned integer, 2 = boolean, 3 = string, |128 = not in multiplayer, |256 = update multi
    int32_t extra;      // for string, is the length
    int32_t min;
    int32_t max;
} cvar_t;

extern cvar_t cvars[];

#endif	// __osdcmds_h__

