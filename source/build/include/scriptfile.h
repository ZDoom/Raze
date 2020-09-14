
#ifndef BUILD_SCRIPTFILE_H_
#define BUILD_SCRIPTFILE_H_

#include "sc_man.h"
#include "filesystem.h"



using scriptfile = FScanner;


inline int32_t scriptfile_getnumber(scriptfile *sf, int32_t *num)
{
	bool res = sf->GetNumber();
	if (res) *num = sf->Number;
	else *num = 0;
	return !res;
}

inline int32_t scriptfile_getdouble(scriptfile *sf, double *num)
{
	bool res = sf->GetFloat();
	if (res) *num = sf->Float;
	else *num = 0;
	return !res;
}

inline int32_t scriptfile_getstring(scriptfile *sf, FString *st)
{
	bool res = sf->GetString();
	if (res) *st = sf->String;
	else *st = "";
	return !res;
}

inline int32_t scriptfile_getsymbol(scriptfile *sf, int32_t *num)
{
	bool res = sf->GetNumber(true);
	if (res) *num = sf->Number;
	else *num = 0;
	return !res;
}

inline FScriptPosition scriptfile_getposition(scriptfile *sf)
{
	return FScriptPosition(*sf);
}

inline int32_t scriptfile_getbraces(scriptfile *sf, FScanner::SavedPos *braceend)
{
	if (sf->CheckString("{"))
	{
		auto here = sf->SavePos();
		sf->SkipToEndOfBlock();
		*braceend = sf->SavePos();
		sf->RestorePos(here);
		return 0;
	}
	else
	{
		sf->ScriptError("'{' expected");
		return -1;
	}
}
inline bool scriptfile_endofblock(scriptfile* sf, FScanner::SavedPos& braceend)
{
	auto here = sf->SavePos();
 	return here.SavedScriptPtr >= braceend.SavedScriptPtr;
}

inline void scriptfile_setposition(scriptfile* sf, const FScanner::SavedPos& pos)
{
	sf->RestorePos(pos);
}

inline scriptfile *scriptfile_fromfile(const char *fn)
{
	int lump = fileSystem.FindFile(fn);
	if (lump < 0) return nullptr;
	auto sc = new FScanner;
	sc->OpenLumpNum(lump);
	sc->SetNoOctals(true);
	sc->SetNoFatalErrors(true);
	return sc;
}

inline void scriptfile_close(scriptfile *sf)
{
	delete sf;
}

inline int32_t scriptfile_addsymbolvalue(scriptfile *sf, char const *name, int32_t val)
{
	sf->AddSymbol(name, val);
	return 1;
}

typedef struct
{
    const char *text;
    int32_t tokenid;
}
tokenlist;


enum
{
    T_EOF = -2,
    T_ERROR = -1,
};

#endif
