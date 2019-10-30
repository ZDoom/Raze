#ifndef __SC_MAN_H__
#define __SC_MAN_H__

#include "zstring.h"
#include "name.h"
#include "basics.h"

class FScanner
{
public:
	struct SavedPos
	{
		const char *SavedScriptPtr;
		int SavedScriptLine;
	};

	// Methods ------------------------------------------------------
	FScanner();
	FScanner(const FScanner &other);
	FScanner(int lumpnum);
	~FScanner();

	FScanner &operator=(const FScanner &other);

	void Open(const char *lumpname);
	void OpenFile(const char *filename);
	void OpenMem(const char *name, const char *buffer, int size);
	void OpenString(const char *name, FString buffer);
	void Close();

	void SetCMode(bool cmode);
	void SetEscape(bool esc);
	void SetStateMode(bool stately);
	void DisableStateOptions();
	const SavedPos SavePos();
	void RestorePos(const SavedPos &pos);

	static FString TokenName(int token, const char *string=NULL);

	bool GetString();
	void MustGetString();
	void MustGetStringName(const char *name);
	bool CheckString(const char *name);

	bool GetToken();
	void MustGetAnyToken();
	void TokenMustBe(int token);
	void MustGetToken(int token);
	bool CheckToken(int token);
	bool CheckTokenId(ENamedName id);

	bool GetNumber();
	void MustGetNumber();
	bool CheckNumber();

	bool GetFloat();
	void MustGetFloat();
	bool CheckFloat();

	void UnGet();

	bool Compare(const char *text);
	int MatchString(const char * const *strings, size_t stride = sizeof(char*));
	int MustMatchString(const char * const *strings, size_t stride = sizeof(char*));
	int GetMessageLine();

	void ScriptError(const char *message, ...) GCCPRINTF(2,3);
	void ScriptMessage(const char *message, ...) GCCPRINTF(2,3);

	bool isText();

	// Members ------------------------------------------------------
	char *String;
	int StringLen;
	int TokenType;
	int Number;
	int64_t BigNumber;
	double Float;
	FName Name;
	int Line;
	bool End;
	bool Crossed;
	int LumpNum;
	FString ScriptName;

protected:
	void PrepareScript();
	void CheckOpen();
	bool ScanString(bool tokens);

	// Strings longer than this minus one will be dynamically allocated.
	static const int MAX_STRING_SIZE = 128;

	bool ScriptOpen;
	FString ScriptBuffer;
	const char *ScriptPtr;
	const char *ScriptEndPtr;
	char StringBuffer[MAX_STRING_SIZE];
	FString BigStringBuffer;
	bool AlreadyGot;
	int AlreadyGotLine;
	bool LastGotToken;
	const char *LastGotPtr;
	int LastGotLine;
	bool CMode;
	BYTE StateMode;
	bool StateOptions;
	bool Escape;
};

enum
{
	TK_SequenceStart = 256,
#define xx(sym,str) sym,
#include "sc_man_tokens.h"
	TK_LastToken
};


//==========================================================================
//
//
//
//==========================================================================

enum
{
	MSG_WARNING,
	MSG_FATAL,
	MSG_ERROR,
	MSG_OPTERROR,
	MSG_DEBUGERROR,
	MSG_DEBUGWARN,
	MSG_DEBUGMSG,
	MSG_LOG,
	MSG_DEBUGLOG,
	MSG_MESSAGE
};

int ParseHex(const char* hex);

#endif //__SC_MAN_H__
