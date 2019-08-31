
#ifndef __view_h__
#define __view_h__

extern short bSubTitles;
extern short nViewTop;
extern short bClip;
extern short nViewBottom;
extern short nViewRight;
extern short nViewLeft;
extern short besttarget;
extern short bCamera;

void InitView();
void SetView1();
void RefreshBackground();
void DrawView();
void MySetView(int x1, int y1, int x2, int y2);
void ResetView();
void NoClip();
void Clip();

extern short nDestVertPan[];
extern short dVertPan[];
extern short nVertPan[];
extern short nQuake[];

extern int nCamerax;
extern int nCameray;
extern int nCameraz;

extern short bTouchFloor;

extern short nChunkTotal;

#endif
