#define JFAUD_INTERNAL
#include "dynlib.hpp"
#include "sysdefs.h"
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#elif defined __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# include <CoreServices/CoreServices.h>
#else
# ifdef SCREWED_UP_CPP
#  include "watcomhax/cstdio"
# else
#  include <cstdio>
# endif
# include <dlfcn.h>
#endif

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

DynamicLibrary::DynamicLibrary(const char *name)
	: handle(NULL)
{
#ifdef _WIN32
	handle = (void*)LoadLibrary((LPCTSTR)name);
#elif defined __APPLE__
	CFBundleRef lib;
	CFURLRef libname, base;
	CFStringRef cfname;
	int i;

	cfname = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);

	for (i=0; i<=2; i++) {
		base = NULL;
		switch (i) {
			case 0:	{	// app bundle private frameworks
				CFBundleRef app = CFBundleGetMainBundle();
				if (!app) continue;
				base = CFBundleCopyPrivateFrameworksURL(app);
			} break;
			case 1:		// user frameworks
			case 2: {	// system frameworks
				FSRef r;
				if (FSFindFolder( i==1 ? kUserDomain : kLocalDomain,
								  kFrameworksFolderType,
								  kDontCreateFolder, &r) < 0) continue;
				base = CFURLCreateFromFSRef(kCFAllocatorDefault, &r);
			} break;
		}
		if (!base) continue;
				
		libname = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, base, cfname, false);
		CFRelease(base);
		if (!libname) continue;

		lib = CFBundleCreate(kCFAllocatorDefault, libname);
		CFRelease(libname);
		if (lib) { handle = (void*)lib; break; }
	}
	
	CFRelease(cfname);
#else
	handle = dlopen(name, RTLD_NOW|RTLD_GLOBAL);
#endif
}

DynamicLibrary::~DynamicLibrary()
{
	if (!handle) return;
#ifdef _WIN32
	FreeLibrary((HMODULE)handle);
#elif defined __APPLE__
	CFRelease((CFBundleRef)handle);
#else
	dlclose(handle);
#endif
}

void *DynamicLibrary::Get(const char *sym)
{
	if (!handle) return NULL;
#ifdef _WIN32
	return (void*)GetProcAddress((HMODULE)handle, (LPCSTR)sym);
#elif defined __APPLE__
	CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, sym, kCFStringEncodingASCII);
	void *p = CFBundleGetFunctionPointerForName((CFBundleRef)handle, s);
	CFRelease(s);
	return p;
#else
	return dlsym(handle, sym);
#endif
}

