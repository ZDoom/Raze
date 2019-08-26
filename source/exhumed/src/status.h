
#ifndef __status_h__
#define __status_h__


extern short nMaskY;
extern short nCounterBullet;
extern short airpages;

void RefreshStatus();
void InitStatus();
void UnMaskStatus();
void MaskStatus();
void LoadStatus();
void SetPlayerItem(short nPlayer, short nItem);
void SetMagicFrame();
void SetHealthFrame(short nVal);
void SetAirFrame();

void MoveStatus();

void DrawStatus();

int BuildStatusAnim(int val, int nFlags);

void SetNextItem(int nPlayer);
void SetPrevItem(int nPlayer);

void SetCounter(short nVal);
void SetCounterImmediate(short nVal);

#endif
