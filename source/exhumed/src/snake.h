
#ifndef __snake_h__
#define __snake_h__

#define kSnakeSprites	8 // or rename to kSnakeParts?

// 32bytes
struct Snake
{
    short nEnemy;	 // nRun
    short nSprites[kSnakeSprites];

    short sC;
    short nRun;

    // array?
    char c[8];
    /*
    char c1;
    char c2;
    char c3;
    char c4;
    char c5;
    char c6;
    char c7;
    char c8;
    */

    short sE;
};

extern Snake SnakeList[];

void InitSnakes();
short GrabSnake();
int BuildSnake(short nPlayer, short zVal);
void FuncSnake(int, int, int);

#endif
