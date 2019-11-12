#include "c_dispatch.h"
#include "cache1d.h"
#include "printf.h"
#include "v_text.h"


//============================================================================
//
// Print secret hints
//
//============================================================================

CCMD(secret)
{
	const char *mapname = argv.argc() < 2? primaryLevel->MapName.GetChars() : argv[1];
	bool thislevel = !stricmp(mapname, primaryLevel->MapName);
	bool foundsome = false;

	int lumpno=Wads.CheckNumForName("SECRETS");
	if (lumpno < 0) return;

	auto lump = Wads.OpenLumpReader(lumpno);
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
					level_info_t *info = FindLevelInfo(mapname);
					const char *ln = !(info->flags & LEVEL_LOOKUPLEVELNAME)? info->LevelName.GetChars() : GStrings[info->LevelName.GetChars()];
					levelname.Format("%s - %s", mapname, ln);
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
 
 SECRET_Save(FileWriter &fil)
 {
 }
 
 SECRET_Load(FileReader &fil)
 {
	 
 }
 
 SECRET_Trigger(int num)
 {
 }
 