
#ifndef __view_h__
#define __view_h__

#include "build.h"

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
void DrawView(int smoothRatio);
void MySetView(int x1, int y1, int x2, int y2);
void ResetView();
void NoClip();
void Clip();

int viewSetInterpolation(int32_t *const posptr);
void viewStopInterpolation(const int32_t * const posptr);
void viewDoInterpolations(int smoothRatio);
void viewUpdateInterpolations(void);
void viewRestoreInterpolations(void);

extern fix16_t nDestVertPan[];
extern short dVertPan[];
extern fix16_t nVertPan[];
extern short nQuake[];

extern int nCamerax;
extern int nCameray;
extern int nCameraz;

extern short bTouchFloor;

extern short nChunkTotal;

static inline int angle_interpolate16(int a, int b, int smooth)
{
    return a + mulscale16(((b+1024-a)&2047)-1024, smooth);
}

#endif
