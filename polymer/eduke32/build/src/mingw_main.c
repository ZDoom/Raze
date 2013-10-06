/**
 * @file main.c
 * Copyright 2012, 2013 MinGW.org project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Extra startup code for applications which do not have a main function
 * of their own (but do have a WinMain). Generally these are GUI
 * applications, but they don't *have* to be.
 */

// Taken from mingwrt-4.0.0 src/libcrt/crt/main.c.

#include "compat.h"

#define ISSPACE(a)	(a == ' ' || a == '\t')

extern int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                           LPSTR szCmdLine, int nShow);

int
main (int argc ATTRIBUTE((unused)), char *argv[] ATTRIBUTE((unused)), char **environment ATTRIBUTE((unused)))
{
  char *szCmd;
  STARTUPINFO startinfo;
  int nRet;

  /* Get the command line passed to the process. */
  szCmd = GetCommandLineA ();
  GetStartupInfoA (&startinfo);

  /* Strip off the name of the application and any leading
   * whitespace. */
  if (szCmd)
    {
      while (ISSPACE (*szCmd))
	{
	  szCmd++;
	}

      /* On my system I always get the app name enclosed
       * in quotes... */
      if (*szCmd == '\"')
	{
	  do
	    {
	      szCmd++;
	    }
	  while (*szCmd != '\"' && *szCmd != '\0');

	  if (*szCmd == '\"')
	    {
	      szCmd++;
	    }
	}
      else
	{
	  /* If no quotes then assume first token is program
	   * name. */
	  while (!ISSPACE (*szCmd) && *szCmd != '\0')
	    {
	      szCmd++;
	    }
	}

      while (ISSPACE (*szCmd))
	{
	  szCmd++;
	}
    }

  nRet = WinMain (GetModuleHandle (NULL), NULL, szCmd,
		  (startinfo.dwFlags & STARTF_USESHOWWINDOW) ?
		  startinfo.wShowWindow : SW_SHOWDEFAULT);

  return nRet;
}

