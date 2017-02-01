// gcc b.c -Lc:/mingw32/lib -lmingw32 -lSDLmain -lSDL

#include <stdio.h>
#include <string.h>
#include "sdl_inc.h"

#include "sdlkeytrans.c"

#undef main

int main(void)
{
    unsigned int i;

    buildkeytranslationtable();

    for (i = 0; i < sizeof(keytranslation); i++) {
        if (i>0) printf(", ");
        if (i%8 == 7) printf("\n");
        printf("%d", keytranslation[i]);
    }

    return 0;
}

