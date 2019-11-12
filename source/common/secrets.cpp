#include "c_dispatch.h"
#include "cache1d.h"
#include "printf.h"
#include "v_text.h"
#include "tarray.h"
#include "c_cvars.h"
#include "v_font.h"
#include "v_draw.h"

// Unlike in GZDoom we have to maintain this list here, because we got different game frontents that all store this info differently.
// So the games will have to report the credited secrets so that this code can keep track of how to display them.
static TArray<int> discovered_secrets;
CVAR(Bool, secret_notify, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);
FString mapfile, maptitle;

//-----------------------------------------------------------------------------
//
// Print secret info (submitted by Karl Murks)
//
//-----------------------------------------------------------------------------

static void PrintSecretString(const char *string, bool thislevel)
{
	const char *colstr = thislevel? TEXTCOLOR_YELLOW : TEXTCOLOR_CYAN;
	if (string != NULL)
	{
		if (*string == '$')
		{
			if (string[1] == 'S' || string[1] == 's')
			{
				auto secnum = (unsigned)strtoull(string+2, (char**)&string, 10);
				if (*string == ';') string++;
				colstr = discovered_secrets.Find(secnum) == discovered_secrets.Size() ? TEXTCOLOR_RED : TEXTCOLOR_GREEN;
			}
		}
		auto brok = V_BreakLines(NewConsoleFont, screen->GetWidth()*95/100, string);

		for (auto &line : brok)
		{
			Printf("%s%s\n", colstr, line.Text.GetChars());
		}
	}
}

//============================================================================
//
// Print secret hints
//
//============================================================================

CCMD(secret)
{
	const char *mapname = argv.argc() < 2? mapfile.GetChars() : argv[1];
	bool thislevel = !stricmp(mapname, mapfile.GetChars());
	bool foundsome = false;

	int lumpno=fileSystem.FindFile("secrets.txt");
	if (lumpno < 0) return;

	auto lump = fileSystem.OpenFileReader(lumpno);
	FString maphdr;
	maphdr.Format("[%s]", mapname);

	FString linebuild;
	char readbuffer[1024];
	bool inlevel = false;

	while (lump.Gets(readbuffer, 1024))
	{
		if (!inlevel)
		{
			if (readbuffer[0] == '[')
			{
				inlevel = !strnicmp(readbuffer, maphdr, maphdr.Len());
				if (!foundsome)
				{
					FString levelname;
					if (thislevel) levelname.Format("%s - %s", mapname, maptitle.GetChars());
					else levelname = mapname;
					Printf(TEXTCOLOR_YELLOW "%s\n", levelname.GetChars());
					size_t llen = levelname.Len();
					levelname = "";
					for(size_t ii=0; ii<llen; ii++) levelname += '-';
					Printf(TEXTCOLOR_YELLOW "%s\n", levelname.GetChars());
					foundsome = true;
				}
			}
			continue;
		}
		else
		{
			if (readbuffer[0] != '[')
			{
				linebuild += readbuffer;
				if (linebuild.Len() < 1023 || linebuild[1022] == '\n')
				{
					// line complete so print it.
					linebuild.Substitute("\r", "");
					linebuild.StripRight(" \t\n");
					PrintSecretString(linebuild, thislevel);
					linebuild = "";
				}
			}
			else inlevel = false;
		}
	}
}
 
void SECRET_Save(FileWriter &fil)
{
	fil.Write("SECR", 4);
	unsigned count = discovered_secrets.Size();
	fil.Write(&count, 4);
	fil.Write(discovered_secrets.Data(), 4 * count);
	fil.Write("RCES", 4);
	
}

bool SECRET_Load(FileReader &fil)
{
	char buf[4];
	unsigned count;
	fil.Read(buf, 4);
	if (memcmp(buf, "SECR", 4)) return false;
	fil.Read(&count, 4);
	discovered_secrets.Resize(count);
	fil.Read(discovered_secrets.Data(), count * 4);
	fil.Read(buf, 4);
	if (memcmp(buf, "RCES", 4)) return false;
	return true;
}

void SECRET_SetMapName(const char *filename, const char *_maptitle)
{
	mapfile = filename;
	maptitle = _maptitle;
}

void  SECRET_Trigger(int num)
{
	if (secret_notify) Printf(PRINT_NONOTIFY, "Secret #%d found\n", num);
	if (discovered_secrets.Find(num) == discovered_secrets.Size())
		discovered_secrets.Push(num);
}

