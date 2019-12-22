// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include "zstring.h"

template <typename T, size_t N>
char(&_ArraySizeHelper(T(&array)[N]))[N];

#define countof( array ) (sizeof( _ArraySizeHelper( array ) )) 

// the dec offsetof macro doesnt work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)alignof(type))->identifier - alignof(type))

bool FileExists (const char *filename);
bool DirExists(const char *filename);
bool DirEntryExists (const char *pathname, bool *isdir = nullptr);

extern	FString progdir;

static void	inline FixPathSeperator (FString &path) { path.ReplaceChars('\\', '/'); }

void 	DefaultExtension (FString &path, const char *extension);
void NormalizeFileName(FString &str);

FString	ExtractFilePath (const char *path);
FString	ExtractFileBase (const char *path, bool keep_extension=false);

struct FScriptPosition;
bool	IsNum (const char *str);		// [RH] added

bool CheckWildcards (const char *pattern, const char *text);

void FormatGUID (char *buffer, size_t buffsize, const GUID &guid);

const char *myasctime ();

int strbin (char *str);
FString strbin1 (const char *start);

void CreatePath(const char * fn);

FString ExpandEnvVars(const char *searchpathstring);
FString NicePath(const char *path);

struct FFileList
{
	FString Filename;
	bool isDirectory;
};

bool IsAbsPath(const char*);


#endif
