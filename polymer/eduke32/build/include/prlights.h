// LIGHTS
#ifndef prlight_h_
# define prlight_h_

#define             PR_MAXLIGHTS            1024
#define             SHADOW_DEPTH_OFFSET     30
#define             PR_MAXLIGHTPRIORITY     6

typedef struct      s_prplanelist {
    struct s_prplane*       plane;
    struct s_prplanelist*   n;
}                   _prplanelist;

#pragma pack(push,1)
typedef struct      s_prlight {
    int32_t         x, y, z, horiz, range;
    int16_t         angle, faderadius, radius, sector;
    uint8_t         color[3], priority;
    int8_t          minshade, maxshade;
    int16_t         tilenum;
    struct          {
        int32_t     emitshadow  : 1;
        int32_t     negative    : 1;
    }               publicflags;
    // internal members
    float           proj[16];
    float           transform[16];
    float           frustum[5 * 4];
    int32_t         rtindex;
    struct          {
        int32_t     active      : 1;
        int32_t     invalidate  : 1;
        int32_t     isinview    : 1;
    }               flags;
    uint32_t        lightmap;
    _prplanelist*   planelist;
    int32_t         planecount;
}                   _prlight;

extern _prlight     prlights[PR_MAXLIGHTS];
extern int32_t      lightcount;
#pragma pack(pop)

#endif
