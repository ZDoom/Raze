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
#include "c_dispatch.h"
#include "md4.h"
#include "hw_sections.h"
#include "mapinfo.h"
#include "gamefuncs.h"

struct usermaphack_t
{
	FString mhkfile;
	FString title;
	uint8_t md4[16]{};
};

static TArray<usermaphack_t> usermaphacks;

void AddUserMapHack(const FString& title, const FString& mhkfile, uint8_t* md4)
{
	usermaphacks.Reserve(1);
	usermaphacks.Last().title = title;
	usermaphacks.Last().mhkfile = mhkfile;
	memcpy(usermaphacks.Last().md4, md4, 16);
}

static int32_t LoadMapHack(const char *filename, SpawnSpriteDef& sprites)
{
	int currentsprite = -1;
	int currentwall = -1;
	int currentsector = -1;
	int numsprites = sprites.sprites.Size();

	FScanner sc;
	int lump = fileSystem.FindFile(filename);
	if (lump < 0)
	{
		return -1;
	}
	sc.OpenLumpNum(lump);
	sprites.sprext.Resize(numsprites);
	memset(sprites.sprext.Data(), 0, sizeof(spriteext_t) * sprites.sprext.Size());

	while (sc.GetString())
	{
		FString token = sc.String;
		auto validateSprite = [&]()
		{
			if (currentsprite < 0 || currentsprite >= numsprites)
			{
				sc.ScriptMessage("Using %s without a valid sprite", token.GetChars());
				return false;
			}
			return true;
		};

		auto validateWall = [&]()
		{
			if (currentwall < 0 || currentwall >= (int)wall.Size())
			{
				sc.ScriptMessage("Using %s without a valid wall", token.GetChars());
				return false;
			}
			return true;
		};

		auto validateSector = [&]()
		{
			if (currentsector < 0 || currentsector >= (int)sector.Size())
			{
				sc.ScriptMessage("Using %s without a valid sector", token.GetChars());
				return false;
			}
			return true;
		};

		if (sc.Compare("sprite"))
		{
			currentwall = -1;
			currentsector = -1;
			if (sc.CheckNumber())
			{
				currentsprite = sc.Number;
				if ((unsigned)currentsprite >= sprites.sprites.Size())
				{
					sc.ScriptMessage("Invalid sprite number %d", currentsprite);
					currentsprite = -1;
				}
			}
			else currentsprite = -1;
		}
		if (sc.Compare("wall"))
		{
			currentsprite = -1;
			currentsector = -1;
			if (sc.CheckNumber())
			{
				currentwall = sc.Number;
				if (!validWallIndex(currentwall))
				{
					sc.ScriptMessage("Invalid wall number %d", currentwall);
					currentwall = -1;
				}
			}
			else currentwall = -1;
		}
		if (sc.Compare("sector"))
		{
			currentsprite = -1;
			currentwall = -1;
			if (sc.CheckNumber())
			{
				currentsector = sc.Number;
				if (!validSectorIndex(currentsector))
				{
					sc.ScriptMessage("Invalid sector number %d", currentsector);
					currentsector = -1;
				}
			}
			else currentsector = -1;
		}
		else if (sc.Compare("sector"))
		{
			if (sc.CheckNumber())
			{
				if (currentsprite != -1 && validateSprite())
				{
					sprites.sprites[currentsprite].sectp = &sector[sc.Number];
				}
			}
		}

		else if (sc.Compare("angleoff") || sc.Compare("angoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].rot.Yaw = mapangle(sc.Number);
		}
		else if (sc.Compare("notmd") || sc.Compare("notmd2") || sc.Compare("notmd3"))
		{
			if (validateSprite())
				sprites.sprext[currentsprite].renderflags |= SPREXT_NOTMD;
		}
		else if (sc.Compare("nomdanim") || sc.Compare("nomd2anim") || sc.Compare("nomd3anim"))
		{
			if (validateSprite())
				sprites.sprext[currentsprite].renderflags |= SPREXT_NOMDANIM;
		}
		else if (sc.Compare("pitch"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].rot.Pitch = mapangle(sc.Number);
		}
		else if (sc.Compare("roll"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].rot.Roll = mapangle(sc.Number);
		}
		else if (sc.Compare("mdxoff") || sc.Compare("mdpivxoff") || sc.Compare("mdpivotxoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].pivot_offset.X = sc.Number;
		}
		else if (sc.Compare("mdyoff") || sc.Compare("mdpivyoff") || sc.Compare("mdpivotyoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].pivot_offset.Y = sc.Number;
		}
		else if (sc.Compare("mdzoff") || sc.Compare("mdpivzoff") || sc.Compare("mdpivotzoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].pivot_offset.Z = sc.Number;
		}
		else if (sc.Compare("mdposxoff") || sc.Compare("mdpositionxoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].position_offset.X = sc.Number * maptoworld;
		}
		else if (sc.Compare("mdposyoff") || sc.Compare("mdpositionyoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].position_offset.X = sc.Number * maptoworld;
		}
		else if (sc.Compare("mdposzoff") || sc.Compare("mdpositionzoff"))
		{
			if (sc.CheckNumber() && validateSprite())
				sprites.sprext[currentsprite].position_offset.X = sc.Number * zmaptoworld;
		}
		else if (sc.Compare("away1"))
		{
			if (validateSprite())
				sprites.sprext[currentsprite].renderflags |= SPREXT_AWAY1;
		}
		else if (sc.Compare("away2"))
		{
			if (validateSprite())
				sprites.sprext[currentsprite].renderflags |= SPREXT_AWAY2;
		}
		else if (sc.Compare("mhkreset"))
		{
			if (validateSprite())
			{
				auto& sx = sprites.sprext[currentsprite];
				sx.rot = {};
				sx.renderflags &= ~(SPREXT_NOTMD | SPREXT_NOMDANIM | SPREXT_AWAY1 | SPREXT_AWAY2);
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

void loadMapHack(const char* filename, const uint8_t* md4, SpawnSpriteDef& sprites)
{
	FString internal = "engine/compatibility/";
	for (int j = 0; j < 16; ++j)
	{
		internal.AppendFormat("%02x", md4[j]);
	}
	LoadMapHack(internal + ".mhk", sprites);
	FString hack = StripExtension(filename) + ".mhk";

	if (LoadMapHack(hack, sprites))
	{
		for (auto& mhk : usermaphacks)
		{
			if (!memcmp(md4, mhk.md4, 16))
			{
				LoadMapHack(mhk.mhkfile, sprites);
			}
		}
	}
}

// Map hacks use MD4 instead of MD5. Oh, well...
CCMD(md4sum)
{
	if (argv.argc() < 2)
	{
		Printf("Usage: md4sum <file> ...\n");
	}
	for (int i = 1; i < argv.argc(); ++i)
	{
		FileReader fr = fileSystem.OpenFileReader(argv[i]);
		if (!fr.isOpen())
		{
			Printf("%s: %s\n", argv[i], strerror(errno));
		}
		else
		{
			auto data = fr.Read();
			uint8_t digest[16];
			md4once(data.data(), data.size(), digest);
			for (int j = 0; j < 16; ++j)
			{
				Printf("%02x", digest[j]);
			}
			Printf(" //*%s\n", argv[i]);
		}
	}
}

