//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "save.h"
#include <stdio.h>
#include <stdarg.h>
#include "init.h"
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <io.h>
#include "engine.h"
#include "exhumed.h"
#include "mmulti.h"
#include "savegamehelp.h"

BEGIN_PS_NS

static TArray<SavegameHelper*> sghelpers(TArray<SavegameHelper*>::NoInit);

int savegame(int nSlot)
{
    auto fw = WriteSavegameChunk("engine");
    fw->Write(&numsectors, sizeof(numsectors));
    fw->Write(sector, sizeof(sectortype) * numsectors);
    fw->Write(&numwalls, sizeof(numwalls));
    fw->Write(wall, sizeof(walltype) * numwalls);
    fw->Write(sprite, sizeof(spritetype) * kMaxSprites);
    fw->Write(headspritesect, sizeof(headspritesect));
    fw->Write(prevspritesect, sizeof(prevspritesect));
    fw->Write(nextspritesect, sizeof(nextspritesect));
    fw->Write(headspritestat, sizeof(headspritestat));
    fw->Write(prevspritestat, sizeof(prevspritestat));
    fw->Write(nextspritestat, sizeof(nextspritestat));

    fw->Write(&tailspritefree, sizeof(tailspritefree));
    fw->Write(&myconnectindex, sizeof(myconnectindex));
    fw->Write(&connecthead, sizeof(connecthead));
    fw->Write(connectpoint2, sizeof(connectpoint2));
    fw->Write(&numframes, sizeof(numframes));
    fw->Write(&randomseed, sizeof(randomseed));
    fw->Write(&numshades, sizeof(numshades));

    fw->Write(&g_visibility, sizeof(g_visibility));
    fw->Write(&parallaxtype, sizeof(parallaxtype));
    fw->Write(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
    fw->Write(&parallaxyscale_override, sizeof(parallaxyscale_override));
    fw->Write(&pskybits_override, sizeof(pskybits_override));

    fw->Write(show2dwall, sizeof(show2dwall));
    fw->Write(show2dsprite, sizeof(show2dsprite));
    fw->Write(show2dsector, sizeof(show2dsector));

    for (auto sgh : sghelpers) sgh->Save();
    return 1; // CHECKME
}

int loadgame(int nSlot)
{
    auto fr = ReadSavegameChunk("engine");
    if (fr.isOpen())
    {
        fr.Read(&numsectors, sizeof(numsectors));
        fr.Read(sector, sizeof(sectortype) * numsectors);
        fr.Read(&numwalls, sizeof(numwalls));
        fr.Read(wall, sizeof(walltype) * numwalls);
        fr.Read(sprite, sizeof(spritetype) * kMaxSprites);
        fr.Read(headspritesect, sizeof(headspritesect));
        fr.Read(prevspritesect, sizeof(prevspritesect));
        fr.Read(nextspritesect, sizeof(nextspritesect));
        fr.Read(headspritestat, sizeof(headspritestat));
        fr.Read(prevspritestat, sizeof(prevspritestat));
        fr.Read(nextspritestat, sizeof(nextspritestat));

        fr.Read(&tailspritefree, sizeof(tailspritefree));
        fr.Read(&myconnectindex, sizeof(myconnectindex));
        fr.Read(&connecthead, sizeof(connecthead));
        fr.Read(connectpoint2, sizeof(connectpoint2));
        fr.Read(&numframes, sizeof(numframes));
        fr.Read(&randomseed, sizeof(randomseed));
        fr.Read(&numshades, sizeof(numshades));

        fr.Read(&g_visibility, sizeof(g_visibility));
        fr.Read(&parallaxtype, sizeof(parallaxtype));
        fr.Read(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
        fr.Read(&parallaxyscale_override, sizeof(parallaxyscale_override));
        fr.Read(&pskybits_override, sizeof(pskybits_override));

        fr.Read(show2dwall, sizeof(show2dwall));
        fr.Read(show2dsprite, sizeof(show2dsprite));
        fr.Read(show2dsector, sizeof(show2dsector));

    }

    for (auto sgh : sghelpers) sgh->Load();

    // reset the sky in case it hasn't been done yet.
    psky_t* pSky = tileSetupSky(0);
    pSky->tileofs[0] = 0;
    pSky->tileofs[1] = 0;
    pSky->tileofs[2] = 0;
    pSky->tileofs[3] = 0;
    pSky->yoffs = 256;
    pSky->lognumtiles = 2;
    pSky->horizfrac = 65536;
    pSky->yscale = 65536;
    parallaxtype = 2;
    g_visibility = 2048;
    ototalclock = totalclock;



    return 1; // CHECKME
}


SavegameHelper::SavegameHelper(const char* name, ...)
{
    Name = name;
    sghelpers.Push(this);
    va_list ap;
    va_start(ap, name);
    for(;;)
    {
        void* addr = va_arg(ap, void*);
        if (!addr) break;
        size_t size = va_arg(ap, size_t);
        Elements.Push(std::make_pair(addr, size));
    }
}

void SavegameHelper::Load()
{
    auto fr = ReadSavegameChunk(Name);
    for (auto& entry : Elements)
    {
        auto read = fr.Read(entry.first, entry.second);
        if (read != entry.second) I_Error("Save game read error in %s", Name.GetChars());
    }
}
void SavegameHelper::Save()
{
    auto fw = WriteSavegameChunk(Name);
    for (auto& entry : Elements)
    {
        auto write = fw->Write(entry.first, entry.second);
        if (write != entry.second) I_Error("Save game write error in %s", Name.GetChars());
    }
}

END_PS_NS
