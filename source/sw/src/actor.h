#pragma once
BEGIN_SW_NS
int DoBeginJump(short SpriteNum);
int DoJump(short SpriteNum);
int DoBeginFall(short SpriteNum);
int DoFall(short SpriteNum);
void KeepActorOnFloor(short SpriteNum);
int DoActorSlide(short SpriteNum);
int DoActorSectorDamage(short SpriteNum);
int DoScaleSprite(short SpriteNum);
END_SW_NS
