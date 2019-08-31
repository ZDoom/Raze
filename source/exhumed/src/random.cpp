
#include "random.h"

int randA = 0;
int randB = 0x11111111;
int randC = 0x1010101;


void InitRandom()
{
    randA = 0;
    randB = 0x11111111;
    randC = 0x1010101;
}

// TODO - checkme
int RandomBit()
{
    randA = (randA >> 1) | (((randA ^ ((randA >> 1) ^ (randA >> 2) ^ (randA >> 31) ^ (randA >> 6) ^ (randA >> 4))) & 1) << 31);
    randB = (randB >> 1) | ((((randB >> 2) ^ (randB >> 30)) & 1) << 30);
    randC = (randC >> 1) | ((((randC >> 1) ^ (randC >> 28)) & 1) << 28);
    return ((randA == 0) & randC | (randB & randA)) & 1;
}

char RandomByte()
{
    char randByte = RandomBit() << 7;
    randByte |= RandomBit() << 6;
    randByte |= RandomBit() << 5;
    randByte |= RandomBit() << 4;
    randByte |= RandomBit() << 3;
    randByte |= RandomBit() << 2;
    randByte |= RandomBit() << 1;
    randByte |= RandomBit();
    return randByte;
}

short RandomWord()
{
    short randWord = RandomByte() << 8;
    randWord |= RandomByte();
    return randWord;
}

int RandomLong()
{
    int randLong = RandomWord() << 16;
    randLong |= RandomWord();
    return randLong;
}

int RandomSize(int nSize)
{
    int randSize = 0;

    while (nSize > 0)
    {
        randSize = randSize * 2 | RandomBit();
        nSize--;
    }

    return randSize;
}
