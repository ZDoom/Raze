#pragma once

// Directory searching routines

// Mirror WIN32_FIND_DATAW in <winbase.h>

struct findstate_t
{
private:
	struct WinData
	{
		uint32_t Attribs;
		uint32_t Times[3 * 2];
		uint32_t Size[2];
		uint32_t Reserved[2];
		wchar_t Name[260];
		wchar_t AltName[14];
	};
	WinData FindData;
	FString UTF8Name;

	friend void *I_FindFirst(const char *filespec, findstate_t *fileinfo);
	friend int I_FindNext(void *handle, findstate_t *fileinfo);
	friend const char *I_FindName(findstate_t *fileinfo);
	friend int I_FindAttr(findstate_t *fileinfo);
};

void *I_FindFirst (const char *filespec, findstate_t *fileinfo);
int I_FindNext (void *handle, findstate_t *fileinfo);
int I_FindClose (void *handle);

const char *I_FindName(findstate_t *fileinfo);
inline int I_FindAttr(findstate_t *fileinfo)
{
	return fileinfo->FindData.Attribs;
}

#define FA_RDONLY	0x00000001
#define FA_HIDDEN	0x00000002
#define FA_SYSTEM	0x00000004
#define FA_DIREC	0x00000010
#define FA_ARCH		0x00000020