/*
** maphack.cpp
**
** Newly implemented map hack loader, based on sc_man.
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/  
#include "build.h"
#include "sc_man.h"
#include "printf.h"

static TArray<usermaphack_t> usermaphacks;

void AddUserMapHack(usermaphack_t& mhk)
{
    usermaphacks.Push(mhk);
}

static int32_t LoadMapHack(const char *filename)
{
    int32_t currentsprite = -1;

    FScanner sc;
    int lump = fileSystem.FindFile(filename);
    if (lump < 0)
    {
        return -1;
    }
    sc.OpenLumpNum(lump);

    while (sc.GetString())
    {
        FString token = sc.String;
        auto validateSprite = [&]()
        {
            if (currentsprite < 0)
            {
                sc.ScriptMessage("Using %s without a valid sprite", token.GetChars());
                return false;
            }
            return true;
        };

        if (sc.Compare("sprite"))
        {
            if (sc.CheckNumber())
            {
                currentsprite = sc.Number;
                if (currentsprite < 0 || currentsprite >= MAXSPRITES)
                {
                    sc.ScriptMessage("Invalid sprite number %d", currentsprite);
                    currentsprite = -1;
                }
            }
            else currentsprite = -1;
        }
        else if (sc.Compare("angleoff") || sc.Compare("angoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].angoff = (int16_t)sc.Number;
        }
        else if (sc.Compare("notmd") || sc.Compare("notmd2") || sc.Compare("notmd3"))
        {
            if (validateSprite())
                spriteext[currentsprite].flags |= SPREXT_NOTMD;
        }
        else if (sc.Compare("nomdanim") || sc.Compare("nomd2anim") || sc.Compare("nomd3anim"))
        {
            if (validateSprite())
                spriteext[currentsprite].flags |= SPREXT_NOMDANIM;
        }
        else if (sc.Compare("pitch"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].pitch = (int16_t)sc.Number;
        }
        else if (sc.Compare("roll"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].roll = (int16_t)sc.Number;
        }
        else if (sc.Compare("mdxoff") || sc.Compare("mdpivxoff") || sc.Compare("mdpivotxoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].pivot_offset.x = sc.Number;
        }
        else if (sc.Compare("mdyoff") || sc.Compare("mdpivyoff") || sc.Compare("mdpivotyoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].pivot_offset.y = sc.Number;
        }
        else if (sc.Compare("mdzoff") || sc.Compare("mdpivzoff") || sc.Compare("mdpivotzoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].pivot_offset.z = sc.Number;
        }
        else if (sc.Compare("mdposxoff") || sc.Compare("mdpositionxoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].position_offset.x = sc.Number;
        }
        else if (sc.Compare("mdposyoff") || sc.Compare("mdpositionyoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].position_offset.x = sc.Number;
        }
        else if (sc.Compare("mdposzoff") || sc.Compare("mdpositionzoff"))
        {
            if (sc.CheckNumber() && validateSprite())
                spriteext[currentsprite].position_offset.x = sc.Number;
        }
        else if (sc.Compare("away1"))
        {
            if (validateSprite())
                spriteext[currentsprite].flags |= SPREXT_AWAY1;
        }
        else if (sc.Compare("away2"))
        {
            if (validateSprite())
                spriteext[currentsprite].flags |= SPREXT_AWAY2;
        }
        else if (sc.Compare("mhkreset"))
        {
            if (validateSprite())
            {
                auto& sx = spriteext[currentsprite];
                sx.angoff = 0;
                sx.flags &= ~(SPREXT_NOTMD | SPREXT_NOMDANIM | SPREXT_AWAY1 | SPREXT_AWAY2);
                sx.pitch = 0;
                sx.roll = 0;
                sx.pivot_offset = {};
                sx.position_offset = {};
            }
        }
        else if (sc.Compare("light"))
        {
            // skip over it - once lights are working this should be reactivated. Assignments were kept as documentation.
            sc.MustGetNumber();
            //light.sector= sc.Number;
            sc.MustGetNumber();
            //light.x= sc.Number;
            sc.MustGetNumber();
            //light.y= sc.Number;
            sc.MustGetNumber();
            //light.z= sc.Number;
            sc.MustGetNumber();
            //light.range= sc.Number;
            sc.MustGetNumber();
            //light.color[0]= sc.Number;
            sc.MustGetNumber();
            //light.color[1]= sc.Number;
            sc.MustGetNumber();
            //light.color[2]= sc.Number;
            sc.MustGetNumber();
            //light.radius= sc.Number;
            sc.MustGetNumber();
            //light.faderadius= sc.Number;
            sc.MustGetNumber();
            //light.angle= sc.Number;
            sc.MustGetNumber();
            //light.horiz= sc.Number;
            sc.MustGetNumber();
            //light.minshade= sc.Number;
            sc.MustGetNumber();
            //light.maxshade= sc.Number;
            sc.MustGetNumber();
            //light.priority= sc.Number;
            sc.MustGetString();
            //light.tilenum= sc.Number;
        }
    }
	return 0;
}

void G_LoadMapHack(const char* filename, const unsigned char* md4)
{
    FString hack = StripExtension(filename) + ".mhk";

    if (LoadMapHack(hack))
    {
        for (auto& mhk : usermaphacks)
        {
            if (!memcmp(md4, mhk.md4, 16))
            {
                LoadMapHack(mhk.mhkfile);
            }
        }
    }
}

