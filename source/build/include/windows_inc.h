// Wrapper for all Windows headers.
// No include guard for this file.

#ifndef _WIN32_IE
# define _WIN32_IE 0x0501
#endif

#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0501
#endif

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif

#ifdef UNREFERENCED_PARAMETER
# undef UNREFERENCED_PARAMETER
#endif

#ifdef NEED_WINSOCK2_H
# include <winsock2.h>
# undef NEED_WINSOCK2_H
#endif

#include <windows.h>

#ifdef NEED_WINDOWSX_H
# include <windowsx.h>
# undef NEED_WINDOWSX_H
#endif

#ifdef NEED_PROCESS_H
# include <process.h>
# undef NEED_PROCESS_H
#endif

#ifdef NEED_SHELLAPI_H
# include <shellapi.h>
# undef NEED_SHELLAPI_H
#endif

#ifdef NEED_SHLWAPI_H
# include <shlwapi.h>
# undef NEED_SHLWAPI_H
#endif

#ifdef NEED_SHLOBJ_H
# include <shlobj.h>
# undef NEED_SHLOBJ_H
#endif

#ifdef NEED_WS2TCPIP_H
# include <ws2tcpip.h>
# undef NEED_WS2TCPIP_H
#endif

#ifdef NEED_COMMCTRL_H
# include <commctrl.h>
# undef NEED_COMMCTRL_H
#endif

#ifdef NEED_MMSYSTEM_H
# include <mmsystem.h>
# undef NEED_MMSYSTEM_H
#endif

#ifdef NEED_DDRAW_H
# define DIRECTDRAW_VERSION  0x0600
# ifndef CINTERFACE
#  define CINTERFACE
# endif
# include "dx/ddraw.h"
# undef NEED_DDRAW_H
#endif

#ifdef NEED_DINPUT_H
# define INITGUID
# define DIRECTINPUT_VERSION 0x0700
# ifndef CINTERFACE
#  define CINTERFACE
# endif
# include "dx/dinput.h"
# undef NEED_DINPUT_H

# if defined (_MSC_VER) || !defined(__cplusplus)
#  define bDIPROP_BUFFERSIZE       MAKEDIPROP(1)
#  define bDIPROP_DEADZONE         MAKEDIPROP(5)
#  define bDIPROP_SATURATION       MAKEDIPROP(6)
# else
#  define bMAKEDIPROP(prop)        ((REFGUID)(prop))
#  define bDIPROP_BUFFERSIZE       bMAKEDIPROP(1)
#  define bDIPROP_DEADZONE         bMAKEDIPROP(5)
#  define bDIPROP_SATURATION       bMAKEDIPROP(6)
# endif

# if defined (_MSC_VER) && defined(__cplusplus)
#  define bREFGUID                 (REFGUID)
#  define bREFIID                  (REFIID)
# else
#  define bREFGUID                 &
#  define bREFIID                  &
# endif

# ifndef DIK_PAUSE
#  define DIK_PAUSE 0xC5
# endif

#endif

#ifdef NEED_DSOUND_H
# define DIRECTSOUND_VERSION  0x0800
# ifndef CINTERFACE
#  define CINTERFACE
# endif
# include "dx/dsound.h"
# undef NEED_DSOUND_H
#endif

#ifdef NEED_CRTDBG_H
# include <crtdbg.h>
# undef NEED_CRTDBG_H
#endif

#ifdef NEED_DBGHELP_H
# if defined _MSC_VER && _MSC_VER < 1300
#  define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers.
// must be XP version of file
// # include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"
#  error "VC6 needs an updated dbghelp.h; see source"
# else
// VC7: ships with updated headers
#  include <dbghelp.h>
# endif
# undef NEED_DBGHELP_H
#endif

#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif
