/* For single inclusion into engine.c only! */

#pragma pack(push,1)
struct sectortypev5
{
    uint16_t wallptr, wallnum;
    int16_t ceilingpicnum, floorpicnum;
    int16_t ceilingheinum, floorheinum;
    int32_t ceilingz, floorz;
    int8_t ceilingshade, floorshade;
    char ceilingxpanning, floorxpanning;
    char ceilingypanning, floorypanning;
    char ceilingstat, floorstat;
    char ceilingpal, floorpal;
    char visibility;
    int16_t lotag, hitag;
    int16_t extra;
};
struct walltypev5
{
    int32_t x, y;
    int16_t point2;
    int16_t picnum, overpicnum;
    int8_t shade;
    int16_t cstat;
    char xrepeat, yrepeat, xpanning, ypanning;
    int16_t nextsector1, nextwall1;
    int16_t nextsector2, nextwall2;
    int16_t lotag, hitag;
    int16_t extra;
};
struct spritetypev5
{
    int32_t x, y, z;
    char cstat;
    int8_t shade;
    char xrepeat, yrepeat;
    int16_t picnum, ang, xvel, yvel, zvel, owner;
    int16_t sectnum, statnum;
    int16_t lotag, hitag;
    int16_t extra;
};
struct sectortypev6
{
    uint16_t wallptr, wallnum;
    int16_t ceilingpicnum, floorpicnum;
    int16_t ceilingheinum, floorheinum;
    int32_t ceilingz, floorz;
    int8_t ceilingshade, floorshade;
    char ceilingxpanning, floorxpanning;
    char ceilingypanning, floorypanning;
    char ceilingstat, floorstat;
    char ceilingpal, floorpal;
    char visibility;
    int16_t lotag, hitag, extra;
};
struct walltypev6
{
    int32_t x, y;
    int16_t point2, nextsector, nextwall;
    int16_t picnum, overpicnum;
    int8_t shade;
    char pal;
    int16_t cstat;
    char xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
};
struct spritetypev6
{
    int32_t x, y, z;
    int16_t cstat;
    int8_t shade;
    char pal, clipdist;
    char xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t picnum, ang, xvel, yvel, zvel, owner;
    int16_t sectnum, statnum;
    int16_t lotag, hitag, extra;
};
#pragma pack(pop)

static int16_t sectorofwallv5(int16_t theline)
{
    int16_t i, startwall, endwall, sucksect;

    sucksect = -1;
    for (i=0; i<numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = startwall + sector[i].wallnum - 1;
        if ((theline >= startwall) && (theline <= endwall))
        {
            sucksect = i;
            break;
        }
    }
    return(sucksect);
}

static void convertv5sectv6(struct sectortypev5 *from, struct sectortypev6 *to)
{
    to->wallptr = from->wallptr;
    to->wallnum = from->wallnum;
    to->ceilingpicnum = from->ceilingpicnum;
    to->floorpicnum = from->floorpicnum;
    to->ceilingheinum = from->ceilingheinum;
    to->floorheinum = from->floorheinum;
    to->ceilingz = from->ceilingz;
    to->floorz = from->floorz;
    to->ceilingshade = from->ceilingshade;
    to->floorshade = from->floorshade;
    to->ceilingxpanning = from->ceilingxpanning;
    to->floorxpanning = from->floorxpanning;
    to->ceilingypanning = from->ceilingypanning;
    to->floorypanning = from->floorypanning;
    to->ceilingstat = from->ceilingstat;
    to->floorstat = from->floorstat;
    to->ceilingpal = from->ceilingpal;
    to->floorpal = from->floorpal;
    to->visibility = from->visibility;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv5wallv6(struct walltypev5 *from, struct walltypev6 *to, int32_t i)
{
    to->x = from->x;
    to->y = from->y;
    to->point2 = from->point2;
    to->nextsector = from->nextsector1;
    to->nextwall = from->nextwall1;
    to->picnum = from->picnum;
    to->overpicnum = from->overpicnum;
    to->shade = from->shade;
    to->pal = sector[sectorofwallv5((int16_t)i)].floorpal;
    to->cstat = from->cstat;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xpanning = from->xpanning;
    to->ypanning = from->ypanning;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv5sprv6(struct spritetypev5 *from, struct spritetypev6 *to)
{
    int16_t j;
    to->x = from->x;
    to->y = from->y;
    to->z = from->z;
    to->cstat = from->cstat;
    to->shade = from->shade;

    j = from->sectnum;
    if ((sector[j].ceilingstat&1) > 0)
        to->pal = sector[j].ceilingpal;
    else
        to->pal = sector[j].floorpal;

    to->clipdist = 32;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xoffset = 0;
    to->yoffset = 0;
    to->picnum = from->picnum;
    to->ang = from->ang;
    to->xvel = from->xvel;
    to->yvel = from->yvel;
    to->zvel = from->zvel;
    to->owner = from->owner;
    to->sectnum = from->sectnum;
    to->statnum = from->statnum;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv6sectv7(struct sectortypev6 *from, sectortype *to)
{
    to->ceilingz = from->ceilingz;
    to->floorz = from->floorz;
    to->wallptr = from->wallptr;
    to->wallnum = from->wallnum;
    to->ceilingpicnum = from->ceilingpicnum;
    to->ceilingheinum = max(min(((int32_t)from->ceilingheinum)<<5,32767),-32768);
    if ((from->ceilingstat&2) == 0) to->ceilingheinum = 0;
    to->ceilingshade = from->ceilingshade;
    to->ceilingpal = from->ceilingpal;
    to->ceilingxpanning = from->ceilingxpanning;
    to->ceilingypanning = from->ceilingypanning;
    to->floorpicnum = from->floorpicnum;
    to->floorheinum = max(min(((int32_t)from->floorheinum)<<5,32767),-32768);
    if ((from->floorstat&2) == 0) to->floorheinum = 0;
    to->floorshade = from->floorshade;
    to->floorpal = from->floorpal;
    to->floorxpanning = from->floorxpanning;
    to->floorypanning = from->floorypanning;
    to->ceilingstat = from->ceilingstat;
    to->floorstat = from->floorstat;
    to->visibility = from->visibility;
    to->fogpal = 0;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
#ifdef NEW_MAP_FORMAT
    to->ceilingbunch = to->floorbunch = -1;
#endif
}

static void convertv6wallv7(struct walltypev6 *from, walltype *to)
{
    to->x = from->x;
    to->y = from->y;
    to->point2 = from->point2;
    to->nextwall = from->nextwall;
    to->nextsector = from->nextsector;
    to->cstat = from->cstat;
    to->picnum = from->picnum;
    to->overpicnum = from->overpicnum;
    to->shade = from->shade;
    to->pal = from->pal;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xpanning = from->xpanning;
    to->ypanning = from->ypanning;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
#ifdef NEW_MAP_FORMAT
    to->upwall = to->dnwall = -1;
#endif
}

static void convertv6sprv7(struct spritetypev6 *from, spritetype *to)
{
    to->x = from->x;
    to->y = from->y;
    to->z = from->z;
    to->cstat = from->cstat;
    to->picnum = from->picnum;
    to->shade = from->shade;
    to->pal = from->pal;
    to->clipdist = from->clipdist;
    to->blend = 0;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xoffset = from->xoffset;
    to->yoffset = from->yoffset;
    to->sectnum = from->sectnum;
    to->statnum = from->statnum;
    to->ang = from->ang;
    to->owner = from->owner;
    to->xvel = from->xvel;
    to->yvel = from->yvel;
    to->zvel = from->zvel;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}
