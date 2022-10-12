//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "blood.h"
#include "filesystem.h"

BEGIN_BLD_NS

// I don't think we still need these.
enum
{
	DICT_LOAD = 0,
	DICT_LOCK = 0,

	kMaxCmdLineDefines = 5,
	kMaxDefines = 1000,
	kMaxParseLevels = 5
};
static int nCmdDefines = 0;
static int nDefines = 0;

static int gParseLevel = 0;
int dword_44CE0[kMaxParseLevels] = { 0, 0, 0, 0, 0 };

// FIXME
unsigned int nBytes = 0;
char buffer[1024];
int scriptValue = 0;

char scriptBuffer[256];

struct define_t
{
	FString _text;
	int   _value;
};

define_t gCmdDefines[kMaxCmdLineDefines];

void addMemoryResource(const char* fileName, int flags, int ID);

struct tag_t {
	const char* _value;
	uint8_t _index;
};

enum eTags
{
	kTag0,
	kTagEnd,
	kTagString,
	kTagConstant,
	kTag4, // string constant?
	kTagComma,
	kTagSemiColon,
	kTagColon,
	kTagEquals,
	kTagHash,
	kTagComment,
	kTagInclude,
	kTagResource,
	kTagAs,
	kTagPreload,
	kTagPrelock,
	kTagData,
	kTagLoad,
	kTagEmit,
	kTagIfDef,
	kTagEndif,
	kTagElse
};

tag_t tags[] =
{
	{ ",", kTagComma },
	{ ";", kTagSemiColon },
	{ ":", kTagColon },
	{ "=", kTagEquals },
	{ "#", kTagHash },
	{ "//", kTagComment },
	{ "INCLUDE", kTagInclude },
	{ "RESOURCE", kTagResource },
	{ "AS", kTagAs },
	{ "PRELOAD", kTagPreload },
	{ "PRELOCK", kTagPrelock },
	{ "DATA", kTagData },
	{ "LOAD", kTagLoad },
	{ "EMIT", kTagEmit },
	{ "%ifdef", kTagIfDef },
	{ "%endif", kTagEndif },
	{ "%else", kTagElse }
};

const int kTagCount = sizeof(tags) / sizeof(tag_t);

int qsort_compar(const void* a, const void* b)
{
	return stricmp((const char*)a, (const char*)b);
}

void SortTags()
{
	qsort(tags, kTagCount, sizeof(tag_t), qsort_compar);
}

void AddCmdDefine(char* text, int value)
{
	assert(nCmdDefines < kMaxCmdLineDefines);

	gCmdDefines[nCmdDefines]._text = text;
	gCmdDefines[nCmdDefines]._value = value;

	nCmdDefines++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void SplitPath(const char* pzPath, char* pzDirectory, char* pzFile, char* pzType)
{
	int const nLength = (int)strlen(pzPath);
	const char* pDot = NULL;
	for (int i = nLength - 1; i >= 0; i--)
	{
		if (pzPath[i] == '/' || pzPath[i] == '\\')
		{
			strncpy(pzDirectory, pzPath, i);
			pzDirectory[i] = 0;
			if (!pDot)
			{
				strcpy(pzFile, pzPath + i + 1);
				strcpy(pzType, "");
			}
			else
			{
				strncpy(pzFile, pzPath + i + 1, pDot - (pzPath + i + 1));
				pzFile[pDot - (pzPath + i + 1)] = 0;
				strcpy(pzType, pDot + 1);
			}

			return;
		}
		else if (pzPath[i] == '.')
		{
			pDot = pzPath + i;
		}
	}
	strcpy(pzDirectory, "/");
	if (!pDot)
	{
		strcpy(pzFile, pzPath);
		strcpy(pzType, "");
	}
	else
	{
		strncpy(pzFile, pzPath, pDot - pzPath);
		pzFile[pDot - pzPath] = 0;
		strcpy(pzType, pDot + 1);
	}
}



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

// 174 bytes
struct RFS
{
private:
	TArray<char> buffer;
	char* _ptr;         // [0]
	char _curChar;      // [4]
	char* _pUnknown2;   // [5]  - some sort of pointer into _ptr?
	char* _pStartLine;  // [9]
	char* _pEnd;        // [13]
	char* _pMark;       // [17]
	char _unknown6;     // [21]
	int  _unknown7;     // [22]
	int  _curLine;      // [26]
	char _fileName[BMAX_PATH]; // [30]

public:
	int Open(int lumpnum);
	void Close();
	void Increment();
	void SkipBeyondValue(char value);
	uint8_t GetNextTag();
	void ScriptError(const char* message);
	void SetMark();
	void UnsetMark();
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int RFS::Open(int lumpnum)
{
	auto hFile = fileSystem.OpenFileReader(lumpnum);
	if (!hFile.isOpen()) {
		Printf("BARF: Error opening file %d", lumpnum);
		return 1;
	}

	int fileSize = (int)hFile.GetLength();
	buffer.Resize(fileSize + 1);
	_ptr = buffer.Data();
	if (_ptr == NULL) {
		Printf("BARF: Not enough memory to read %d", lumpnum);
		return 1;
	}

	hFile.Read(_ptr, fileSize);
	buffer[fileSize] = '\n';

	_curLine = 0;
	_pUnknown2 = _ptr;
	_curChar = '\n';
	_pEnd = &_ptr[fileSize];

	return 0;
}

void RFS::Close()
{
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void RFS::Increment()
{
	if (_curChar == '\n') {
		_curLine++;
		_pStartLine = _pUnknown2;
	}

	if (_pUnknown2 >= _pEnd) {
		_curChar = 0;
	}
	else {
		_curChar = *_pUnknown2; // grabs the next char
		_pUnknown2++; // increment pointer into char data
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void RFS::SkipBeyondValue(char nVal)
{
	while (_curChar && _curChar != nVal) {
		Increment();
	}
}

void RFS::SetMark()
{
	_pMark = _pUnknown2;
	_unknown6 = _curChar;
	_unknown7 = _curLine;
}

// inverse of the above function
void RFS::UnsetMark()
{
	_pUnknown2 = _pMark;
	_curChar = _unknown6;
	_curLine = _unknown7;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void RFS::ScriptError(const char* message)
{
	// TODO
	TArray<char> msg;

	char* p = _pStartLine;
	while (*p != '\n')
	{
		if (isprint((uint8_t) *p))
			msg.Push(*p);
		else
			msg.Push(' ');
		p++;
	}

	msg.Push('\n');

	p = _pStartLine;

	while (p < _pMark)
	{
		msg.Push(' ');
		p++;
	}

	msg.Push('^');
	msg.Push(0);

	Printf("Error in %s line %d: %s\n\n%s", _fileName, _curLine, message, msg.Data());
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

uint8_t RFS::GetNextTag()
{
	// skip any space characters
	do {
		Increment();
	} while (isspace((uint8_t)_curChar));

	if (_curChar == '\0') {
		return kTagEnd;
	}

	SetMark();

	// Path A
	if (_curChar == '"')
	{
		Increment();

		int i = 0;

		// section 1
		while (1) {

			if (_curChar == '\0' || _curChar == '"') {
				scriptBuffer[i] = '\0';
				return kTagString;
			}

			if (i == 256) {
				ScriptError("String exceeds maximum string length");
				break;
			}

			scriptBuffer[i] = _curChar;
			i++;
			Increment();
		}

		// section 2
		while (1)
		{
			if (_curChar == '\0' || _curChar == '"') {
				return kTag0;
			}

			Increment();
		}
	}
	else
	{
		scriptValue = 0;
		bool isNegative = false; // or 'isSigned' ?

		// is it a negative number?
		if (_curChar == '-')
		{
			Increment();

			isNegative = true;

			if (!isdigit((uint8_t)_curChar)) {
				UnsetMark();
			}
		}

		if (isdigit((uint8_t)_curChar))
		{
			// left path
			if (_curChar == '0')
			{
				Increment();

				// handle a hex value
				if (toupper(_curChar) == 'X')
				{
					// orange loop
					while (1)
					{
						Increment();
						if (!isxdigit((uint8_t)_curChar)) { // isxdigit() checks for a hex value
							break;
						}

						// hex version of atoi?
						scriptValue *= 16;
						if (!isdigit((uint8_t)_curChar)) {
							scriptValue += toupper((uint8_t)_curChar) - 55;
						}
						else {
							scriptValue += _curChar - '0';
						}

						SetMark();
					}

					UnsetMark();
					if (isNegative) {
						scriptValue = -scriptValue;
					}

					return kTagConstant;
				}

				UnsetMark();
			}

			// the loop
			while (isdigit(_curChar))
			{
				// atoi implementation
				scriptValue = scriptValue * 10 + _curChar - '0';
				SetMark();
				Increment();
			}

			UnsetMark();
			if (isNegative) {
				scriptValue = -scriptValue;
			}

			return kTagConstant;
		}
		else
		{
			// BLUEISH PATH
			int ebp = 0; // v11
			int i = 0;

			// blue loop #1
			while (1)
			{
				scriptBuffer[ebp] = _curChar;
				ebp++;
				int eax = -1;

				// blue loop #2
				for (i = 0; i < kTagCount; i++)
				{
					//if (eax >= 0) {
					if (eax == 0) {
						break;
					}

					// eax = strnicmp(tags[i]._value, scriptBuffer, ebp);
					eax = strnicmp(scriptBuffer, tags[i]._value, ebp);

					//if (eax >= 0) {
					if (eax == 0) {
						break;
					}
				}

				if (eax > 0 || i == kTagCount) {
					break;
				}

				if (eax == 0 && (int)strlen(tags[i]._value) == ebp)
				{
					scriptBuffer[ebp] = 0;
					return tags[i]._index;
				}

				Increment();
			}

			UnsetMark();

			i = 0;

			while (isalnum((uint8_t)_curChar))
			{
				scriptBuffer[i] = _curChar;
				SetMark();
				i++;
				Increment();
			}

			UnsetMark();
			scriptBuffer[i] = 0;
			return kTag4;
		}
	}

	//	qAssert(1==0); // TODO - what to return here
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ParseScript(int lumpnum)
{
	char text[256];
	char char256_1[256];
	char char256_2[256];
	char fileName[BMAX_PATH];
	char inp[BMAX_PATH];
	//char zScriptDirectory[BMAX_PATH], zTemp1[BMAX_PATH], zTemp2[BMAX_PATH];

	//SplitPath(scriptFileName, zScriptDirectory, zTemp1, zTemp2);

	RFS rfs;

	// AddExtension(name, ".RFS");
	if (rfs.Open(lumpnum))
	{
		return;
	}

	gParseLevel = 0;
	dword_44CE0[0] = 0;

	bool parsing = true;

	while (parsing)
	{
		// START LOOP. to be fixed later
	START:

		uint8_t tag = rfs.GetNextTag();

		switch (tag)
		{
		default:
		{
			break;
		}
		case kTagEnd:
		{
			parsing = false;
			break;
		}
		case kTagComment:
		{
			// skip to next line
			rfs.SkipBeyondValue('\n');
			break;
		}
		case kTagEmit: // minty/light green colour
		{
			tag = rfs.GetNextTag();
			if (tag != kTag4)
			{
				rfs.ScriptError("Symbol name expected");
				rfs.SkipBeyondValue(';');
				break;
			}
			else
			{
				strcpy(char256_2, scriptBuffer);
				tag = rfs.GetNextTag();
				if (tag != kTagEquals)
				{
					rfs.ScriptError("Missing '='");
					rfs.SkipBeyondValue(';');
					break;
				}

				tag = rfs.GetNextTag();
				if (tag != kTagConstant)
				{
					rfs.ScriptError("Constant expected");
					rfs.SkipBeyondValue(';');
					break;
				}
				else
				{
					//AddDefine(char256_2, scriptValue);
					rfs.SkipBeyondValue('\n');
				}
			}
			[[fallthrough]];
		}
		case kTagResource: // really light blue..
		{
			if (kTagString != rfs.GetNextTag()) {
				rfs.ScriptError("String constant exected");
				rfs.SkipBeyondValue('\n');
				break;
			}

			strcpy(inp, scriptBuffer);
			uint8_t nFlags = 0;
			int ID = 0;
			bool isDefine = false;

			tag = rfs.GetNextTag();
			if (tag == kTagAs)
			{
				tag = rfs.GetNextTag();
				if (tag == kTag4)
				{
					strcpy(text, scriptBuffer);

					if (rfs.GetNextTag() != kTagEquals)
					{
						rfs.ScriptError("Missing '='");
						rfs.SkipBeyondValue(';');
						break;
					}

					isDefine = true;
					tag = rfs.GetNextTag();
				}

				if (tag != kTagConstant)
				{
					rfs.ScriptError("Constant expected");
					rfs.SkipBeyondValue(';');
					break;
				}

				if (isDefine) {
					//AddDefine(text, scriptValue);
				}

				ID = scriptValue;
				tag = rfs.GetNextTag();
			}

			//if (!bNoEncrypt) {
			//	nFlags |= kResFlagIsEncrypted;
			//}

			while (tag == kTagComma)
			{
				tag = rfs.GetNextTag();

				if (tag == kTagPreload) {
					nFlags |= DICT_LOAD;
				}
				else if (tag == kTagPrelock) {
					nFlags |= DICT_LOCK;
				}
				else {
					rfs.ScriptError("Unrecognized flag");
					rfs.SkipBeyondValue(';');
					goto START; // FIXME
				}

				tag = rfs.GetNextTag();
			}

			if (tag != kTagSemiColon)
			{
				rfs.ScriptError("';' expected");
				rfs.SkipBeyondValue(';');
				break;
			}

			if (dword_44CE0[gParseLevel] == 0)
			{
				// In the RFS files I have seen the outermost directory is not part of what goes into the file system.
				auto inp1 = strpbrk(inp, "/\\");
				if (!inp1 || !fileSystem.CreatePathlessCopy(inp1 + 1, ID, nFlags))
				{
					// GDX spports this so we should, too.
					fileSystem.CreatePathlessCopy(inp, ID, nFlags);
				}
			}

			break;
		}
		case kTagIfDef: // purplish colour
		{
			tag = rfs.GetNextTag();
			if (tag != kTag4)
			{
				rfs.ScriptError("Parameter error in ifdef");
				rfs.SkipBeyondValue('\n');
				break;
			}
			else
			{
				rfs.SetMark();
				strcpy(char256_1, scriptBuffer);

				bool bGotDefine = false;

				// check if this was defined via command prompt arguments
				for (int i = 0; i < nCmdDefines; i++)
				{
					if (stricmp(gCmdDefines[i]._text, char256_1) == 0) { // string is equivalent
						bGotDefine = true;
						break;
					}
				}

				// loc_11FC3:
				gParseLevel++;
				assert(gParseLevel < kMaxParseLevels);

				if (bGotDefine) {
					dword_44CE0[gParseLevel] = dword_44CE0[gParseLevel - 1];
				}
				else {
					dword_44CE0[gParseLevel] = 1;
				}
			}
			break;
		}
		case kTagElse: // pinky colour
		{
			if (gParseLevel)
			{
				// loc_12066:
				if (dword_44CE0[gParseLevel - 1] == 0) {
					if (dword_44CE0[gParseLevel] == 0) {
						dword_44CE0[gParseLevel] = 1;
					}

					rfs.SkipBeyondValue('\n');
					break;
				}
			}
			else {
				rfs.ScriptError("Unexpected else");
				rfs.SkipBeyondValue('\n');
				break;
			}
			break;
		}
		case kTagEndif: // poo coloured
		{
			if (gParseLevel) {
				gParseLevel--;
				rfs.SkipBeyondValue('\n');
				break;
			}
			else
			{
				rfs.ScriptError("Unexpected Endif");
				rfs.SkipBeyondValue('\n');
				break;
			}
		}
		case kTagHash: // gold colour
		{
			tag = rfs.GetNextTag();
			if (tag == kTagInclude)
			{
				tag = rfs.GetNextTag();
				if (tag != kTagString)
				{
					rfs.ScriptError("String constant exected");
					// why no SkipBeyondValue?
					break;
				}
				else
				{
					// too dangerous if we want to cumulatively collect all RFS files
					//fileSystem.Rehash();
					//ParseScript(scriptBuffer);
				}
			}
			break;
		}
		case kTagData: // eg:        data "AMB1.SFX" as 1:	80, 0x10000, 0x0, 1, -1, "amb1";
		{
			// green coloured section
			if (rfs.GetNextTag() != kTagString) {
				rfs.ScriptError("String constant expected");
				rfs.SkipBeyondValue(';');
				break;
			}

			// eg strcpy(fileName, "AMB1.SFX");
			strcpy(fileName, scriptBuffer);

			uint8_t nFlags = 0;
			int ID = 0;

			bool isDefine = false;

			tag = rfs.GetNextTag();

			// process an ID section
			if (tag == kTagAs)
			{
				tag = rfs.GetNextTag();
				if (tag == kTag4)
				{
					strcpy(fileName, scriptBuffer);

					tag = rfs.GetNextTag();
					if (tag != kTagEquals) {
						rfs.ScriptError("Missing '='");
						rfs.SkipBeyondValue(';');
						break;
					}

					isDefine = true;
					tag = rfs.GetNextTag();
				}

				if (tag != kTagConstant)
				{
					rfs.ScriptError("Constant Expected");
					rfs.SkipBeyondValue(';');
					break;
				}
				else {
					//if (isDefine) {
					//    AddDefine(fileName, scriptValue);
					//}

					ID = scriptValue;
					tag = rfs.GetNextTag();
				}
			}

			if (tag == kTagComma)
			{
				// process all sections on this line that are comma separated
				while (1)
				{
					tag = rfs.GetNextTag();
					if (tag == kTagPreload)
					{
						nFlags |= DICT_LOAD;
						tag = rfs.GetNextTag();

						if (tag == kTagComma) {
							continue;
						}
					}
					else if (tag == kTagPrelock)
					{
						nFlags |= DICT_LOCK;
						tag = rfs.GetNextTag();

						if (tag == kTagComma) {
							continue;
						}
					}
					else
					{
						rfs.ScriptError("Unrecognized flag");
						rfs.SkipBeyondValue(';');
						goto START; // FIXME
					}
				}
			}

			// loc_12471:
			if (tag != kTagColon) // marked orange in IDA
			{
				while (1)
				{
					if (tag == kTagPreload)
					{
						nFlags |= DICT_LOAD;
						tag = rfs.GetNextTag();

						if (tag == kTagColon) {
							break;
						}
					}
					else if (tag == kTagPrelock)
					{
						nFlags |= DICT_LOCK;
						tag = rfs.GetNextTag();

						if (tag == kTagColon) {
							break;
						}
					}
					else {
						rfs.ScriptError("':' expected");
						rfs.SkipBeyondValue(';');
						goto START; // FIXME
					}
				}
			}

			nBytes = 0;

			// yellow loop
			while (1)
			{
				tag = rfs.GetNextTag();

				switch (tag)
				{
				case kTagString:
				{
					memcpy(&buffer[nBytes], scriptBuffer, strlen(scriptBuffer) + 1);
					nBytes += (int)strlen(scriptBuffer) + 1;
					break;
				}
				case kTagConstant:
				{
					memcpy(&buffer[nBytes], &scriptValue, sizeof(scriptValue));
					nBytes += sizeof(scriptValue);
					break;
				}
				default:
				{
					rfs.ScriptError("Constant expected");
					rfs.SkipBeyondValue(';');
					goto START; // FIXME
				}
				}

				tag = rfs.GetNextTag();
				if (tag != kTagComma) {
					break;
				}
			}

			if (tag != kTagSemiColon) {
				rfs.ScriptError("Semicolon expected");
				rfs.SkipBeyondValue(';');
				break;
			}
			else
			{
				if (dword_44CE0[gParseLevel] == 0) {
					addMemoryResource(fileName, nFlags, ID);
				}
			}
			break;
		}
		}
	}

	//CreateHeader();
	rfs.Close();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void addMemoryResource(const char* filePath, int flags, int ID)
{
	char zDirectory[BMAX_PATH];
	char zFilename[BMAX_PATH];
	char zType[BMAX_PATH];

	SplitPath(filePath, zDirectory, zFilename, zType);

	fileSystem.AddFromBuffer(zFilename, zType, buffer, nBytes, ID, flags);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ReadAllRFS()
{
	bool found = false;
	auto numf = fileSystem.GetNumEntries();
	for (int i = 0; i < numf; i++)
	{
		auto rl = fileSystem.GetResourceType(i);
		if (!stricmp(rl, "RFS"))
		{
			ParseScript(i);
			found = true;
		}
	}
	if (found) fileSystem.InitHashChains();
}
END_BLD_NS
