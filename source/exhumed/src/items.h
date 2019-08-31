
#ifndef __items_h__
#define __items_h__

enum
{
    kItemHeart = 0,
    kItemInvincibility,
    kItemTorch,
    kItemDoubleDamage,
    kItemInvisibility,
    kItemMask,
};

extern short nItemMagic[];

void BuildItemAnim(short nSprite);
void DestroyItemAnim(short nSprite);
void ItemFlash();
void FillItems(short nPlayer);
void UseEye(short nPlayer);
void UseMask(short nPlayer);
void UseTorch(short nPlayer);
void UseHeart(short nPlayer);
void UseScarab(short nPlayer);
void UseHand(short nPlayer);
void UseItem(short nPlayer, short nItem);
void UseCurItem(short nPlayer);
int GrabItem(short nPlayer, short nItem);
void DropMagic(short nSprite);
void InitItems();
void StartRegenerate(short nSprite);
void DoRegenerates();

#endif
