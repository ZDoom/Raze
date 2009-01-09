void GAME_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal);
void GAME_drawosdstr(int32_t x, int32_t y, char *ch, int32_t len, int32_t shade, int32_t pal);
void GAME_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress);
int32_t GAME_getcolumnwidth(int32_t w);
int32_t GAME_getrowheight(int32_t w);
void GAME_clearbackground(int32_t c, int32_t r);
void GAME_onshowosd(int32_t shown);

extern int32_t osdhightile;

#define OSDCHAR_WIDTH 8

