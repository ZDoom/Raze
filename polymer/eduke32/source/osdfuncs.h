void GAME_drawosdchar(int x, int y, char ch, int shade, int pal);
void GAME_drawosdstr(int x, int y, char *ch, int len, int shade, int pal);
void GAME_drawosdcursor(int x, int y, int type, int lastkeypress);
int GAME_getcolumnwidth(int w);
int GAME_getrowheight(int w);
void GAME_clearbackground(int c, int r);
void GAME_onshowosd(int shown);

extern int osdhightile;

#define OSDCHAR_WIDTH 8

