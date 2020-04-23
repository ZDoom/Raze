#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include "zstring.h"



//==========================================================================
//
// QueryPathKey
//
// Returns the value of a registry key into the output variable value.
//
//==========================================================================

bool I_QueryPathKey(const wchar_t* keypath, const wchar_t* valname, FString& value)
{
	HKEY pathkey;
	DWORD pathtype;
	DWORD pathlen;
	LONG res;

	value = "";
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, keypath, 0, KEY_QUERY_VALUE, &pathkey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(pathkey, valname, 0, &pathtype, NULL, &pathlen) &&
			pathtype == REG_SZ && pathlen != 0)
		{
			// Don't include terminating null in count
			TArray<wchar_t> chars(pathlen + 1, true);
			res = RegQueryValueEx(pathkey, valname, 0, NULL, (LPBYTE)chars.Data(), &pathlen);
			if (res == ERROR_SUCCESS) value = FString(chars.Data());
		}
		RegCloseKey(pathkey);
	}
	return value.IsNotEmpty();
}

