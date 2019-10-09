#ifndef colormap_public_
#define colormap_public_

BEGIN_SW_NS

extern unsigned char DefaultPalette[];

void MapColors(short num,COLOR_MAP cm,short create);
void InitPalette(void);
void SetPaletteToVESA(unsigned char *pal);
void set_pal(unsigned char *pal);
void GetPaletteFromVESA(unsigned char *pal);
void InitPalette(void);

END_SW_NS

#endif
