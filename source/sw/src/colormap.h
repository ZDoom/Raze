#ifndef colormap_public_
#define colormap_public_

extern unsigned char DefaultPalette[];

void MapColors(short num,COLOR_MAP cm,short create);
void InitPalette(void);
void SetPaletteToVESA(unsigned char *pal);
void set_pal(unsigned char *pal);
void GetPaletteFromVESA(unsigned char *pal);
void InitPalette(void);

#endif
