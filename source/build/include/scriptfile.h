
#ifndef BUILD_SCRIPTFILE_H_
#define BUILD_SCRIPTFILE_H_

#include "sc_man.h"

typedef struct {
	char *textbuf;
	uint32_t textlength;
	char *ltextptr;		// pointer to start of the last token fetched (use this for line numbers)
	char *textptr;
	char *eof;
	char *filename;
	int32_t linenum;
	int32_t *lineoffs;
} scriptfile;

char *scriptfile_gettoken(scriptfile *sf);
int32_t scriptfile_getnumber(scriptfile *sf, int32_t *num);
int32_t scriptfile_getdouble(scriptfile *sf, double *num);
int32_t scriptfile_getstring(scriptfile *sf, FString *st);
int scriptfile_getsymbol(scriptfile *sf, int32_t *num);
int32_t scriptfile_getlinum(const scriptfile *sf, const char *ptr);
int32_t scriptfile_getbraces(scriptfile *sf, FScanner::SavedPos *braceend);
inline bool scriptfile_endofblock(scriptfile* sf, FScanner::SavedPos& braceend)
{
	return sf->textptr >= braceend.SavedScriptPtr;
}
void scriptfile_setposition(scriptfile* sf, const FScanner::SavedPos& pos);

scriptfile *scriptfile_fromfile(const char *fn);
void scriptfile_close(scriptfile *sf);
int scriptfile_eof(scriptfile *sf);

int32_t scriptfile_getsymbolvalue(char const *name, int32_t *val);
int32_t scriptfile_addsymbolvalue(char const *name, int32_t val);
void scriptfile_clearsymbols(void);

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


int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens);

#endif
