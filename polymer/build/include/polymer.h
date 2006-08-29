// blah

#ifndef _polymer_h_
#define _polymer_h_

// geometry container
typedef struct  DNF
{
    double      v1[3], v2[3], v3[3];
    double      u1[2], u2[2], u3[2];
    short       picnum;
}               _poly;

void    polymer_glinit(void);
void    polymer_init(void);
void    polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum);

#endif // !_polymer_h_
