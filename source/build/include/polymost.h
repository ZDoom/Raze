#ifndef polymost_h_
# define polymost_h_


#include "mdsprite.h"

extern tspritetype pm_tsprite[MAXSPRITESONSCREEN];
extern int pm_spritesortcnt;
extern int pm_smoothratio;


namespace Polymost
{
extern float gtang;
extern double gxyaspect;
extern float grhalfxdown10x;
extern float gcosang, gsinang, gcosang2, gsinang2;

extern void Polymost_prepare_loadboard(void);


//void phex(char v, char *s);
void polymost_drawsprite(int32_t snum);
void polymost_drawmaskwall(int32_t damaskwallcnt);
void polymost_dorotatespritemodel(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend, int32_t uniqid);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);
void polymost_prepareMirror(int32_t dax, int32_t day, int32_t daz, fixed_t daang, fixed_t dahoriz, int16_t mirrorWall);
void polymost_completeMirror();
void polymost_deletesprite(int num);

int32_t polymost_maskWallHasTranslucency(walltype const * const wall);
int32_t polymost_spriteHasTranslucency(spritetype const * const tspr);

void polymost_glreset(void);
void polymost_scansector(int32_t sectnum);

extern float curpolygonoffset;

}

void   renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fixed_t daang, fixed_t dahoriz, int16_t dawall,
    int32_t* tposx, int32_t* tposy, fixed_t* tang);
void   renderCompleteMirror(void);

int32_t renderDrawRoomsQ16(int32_t daposx, int32_t daposy, int32_t daposz, fixed_t daang, fixed_t dahoriz, int dacursectnum, bool fromoutside);

void   renderDrawMasks(void);

// PLAG: line utility functions
typedef struct s_equation
{
    float a, b, c;
} _equation;

void    renderSetRollAngle(float rolla);


// these are defined in engine.cpp.
extern int16_t globalpicnum;
extern float fcosglobalang, fsinglobalang;
extern float fydimen, fviewingrange;
extern int32_t viewingrangerecip;

// Flags of the <dameth> argument of various functions
enum {
    DAMETH_NOMASK = 0,
    DAMETH_MASK = 1,
    DAMETH_TRANS1 = 2,
    DAMETH_TRANS2 = 3,

    DAMETH_MASKPROPS = 3,

    DAMETH_CLAMPED = 4,
	DAMETH_MODEL = 8,
	DAMETH_SKY = 16,

    DAMETH_WALL = 32,  // signals a texture for a wall (for r_npotwallmode)

    // used internally by polymost_domost
    DAMETH_BACKFACECULL = -1,
};

#endif
