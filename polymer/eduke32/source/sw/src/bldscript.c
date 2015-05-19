//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2005 - 3D Realms Entertainment

This file is NOT part of Shadow Warrior version 1.2
However, it is either an older version of a file that is, or is
some test code written during the development of Shadow Warrior.
This file is provided purely for educational interest.

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// scriplib.c
#include "build.h"
#include "editor.h"
#include "cache1d.h"

#include "names2.h"
#include "game.h"

#include "parse.h"

#define PATHSEPERATOR   '\\'


//#define COMPUTE_TOTALS	1

/*
=============================================================================

                        ABNORMAL TERMINATION

=============================================================================
*/
void Error(char *error, ...)
{
    va_list argptr;

    va_start(argptr,error);
    vprintf(error,argptr);
    va_end(argptr);
    printf("\n");
    exit(1);
}



/*
=============================================================================

                        PARSING STUFF

=============================================================================
*/

char    token[MAXTOKEN];
char    *scriptbuffer,*script_p,*scriptend_p;
int     grabbed;
int     scriptline;
SWBOOL    endofscript;
SWBOOL    tokenready;                     // only TRUE if UnGetToken was just called

/*
==============
=
= LoadScriptFile
=
==============
*/

SWBOOL LoadScriptFile(char *filename)
{
    int size, readsize;
    int fp;


    if ((fp=kopen4load(filename,0)) == -1)
    {
        // If there's no script file, forget it.
        return FALSE;
    }

    size = kfilelength(fp);

    scriptbuffer = (char *)malloc(size);

    ASSERT(scriptbuffer != NULL);

    readsize = kread(fp, scriptbuffer, size);

    kclose(fp);

    ASSERT(readsize == size);


    // Convert filebuffer to all upper case
    //strupr(scriptbuffer);

    script_p = scriptbuffer;
    scriptend_p = script_p + size;
    scriptline = 1;
    endofscript = FALSE;
    tokenready = FALSE;
    return TRUE;
}


/*
==============
=
= UnGetToken
=
= Signals that the current token was not used, and should be reported
= for the next GetToken.  Note that

GetToken (TRUE);
UnGetToken ();
GetToken (FALSE);

= could cross a line boundary.
=
==============
*/

void UnGetToken(void)
{
    tokenready = TRUE;
}


/*
==============
=
= GetToken
=
==============
*/

void GetToken(SWBOOL crossline)
{
    char    *token_p;

    if (tokenready)                         // is a token already waiting?
    {
        tokenready = FALSE;
        return;
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
            Error("Line %i is incomplete\n",scriptline);
        endofscript = TRUE;
        return;
    }

//
// skip space
//
skipspace:
    while (*script_p <= 32)
    {
        if (script_p >= scriptend_p)
        {
            if (!crossline)
                Error("Line %i is incomplete\n",scriptline);
            endofscript = TRUE;
            return;
        }
        if (*script_p++ == '\n')
        {
            if (!crossline)
                Error("Line %i is incomplete\n",scriptline);
            scriptline++;
        }
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
            Error("Line %i is incomplete\n",scriptline);
        endofscript = TRUE;
        return;
    }

    if (*script_p == '#')   // # is comment field
    {
        if (!crossline)
            Error("Line %i is incomplete\n",scriptline);
        while (*script_p++ != '\n')
            if (script_p >= scriptend_p)
            {
                endofscript = TRUE;
                return;
            }
        goto skipspace;
    }

//
// copy token
//
    token_p = token;

    while (*script_p > 32 && *script_p != '#')
    {
        *token_p++ = *script_p++;
        if (script_p == scriptend_p)
            break;
        ASSERT(token_p != &token[MAXTOKEN]);
//			Error ("Token too large on line %i\n",scriptline);
    }

    *token_p = 0;
}


/*
==============
=
= TokenAvailable
=
= Returns true if there is another token on the line
=
==============
*/

SWBOOL TokenAvailable(void)
{
    char    *search_p;

    search_p = script_p;

    if (search_p >= scriptend_p)
        return FALSE;

    while (*search_p <= 32)
    {
        if (*search_p == '\n')
            return FALSE;
        search_p++;
        if (search_p == scriptend_p)
            return FALSE;

    }

    if (*search_p == '#')
        return FALSE;

    return TRUE;
}

void DefaultExtension(char *path, char *extension)
{
    char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
    src = path + strlen(path) - 1;

    while (*src != '\\' && src != path)
    {
        if (*src == '.')
            return;                 // it has an extension
        src--;
    }

    strcat(path, extension);
}


void DefaultPath(char *path, char *basepath)
{
    char    temp[128];

    if (path[0] == '\\')
        return;                                                 // absolute path location
    strcpy(temp,path);
    strcpy(path,basepath);
    strcat(path,temp);
}


void    StripFilename(char *path)
{
    int             length;

    length = strlen(path)-1;
    while (length > 0 && path[length] != PATHSEPERATOR)
        length--;
    path[length] = 0;
}


void ExtractFileBase(char *path, char *dest)
{
    char    *src;
    int             length;

    src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
    while (src != path && *(src-1) != '\\')
        src--;

//
// copy up to eight characters
//
    memset(dest,0,8);
    length = 0;
    while (*src && *src != '.')
    {
        if (++length == 9)
            Error("Filename base of %s >8 chars",path);
        *dest++ = toupper(*src++);
    }
}


/*
==============
=
= ParseNum / ParseHex
=
==============
*/

int ParseHex(char *hex)
{
    char    *str;
    int    num;

    num = 0;
    str = hex;

    while (*str)
    {
        num <<= 4;
        if (*str >= '0' && *str <= '9')
            num += *str-'0';
        else if (*str >= 'a' && *str <= 'f')
            num += 10 + *str-'a';
        else if (*str >= 'A' && *str <= 'F')
            num += 10 + *str-'A';
        else
            Error("Bad hex number: %s",hex);
        str++;
    }

    return num;
}


int ParseNum(char *str)
{
    if (str[0] == '$')
        return ParseHex(str+1);
    if (str[0] == '0' && str[1] == 'x')
        return ParseHex(str+2);
    return atol(str);
}




// voxelarray format is:
//      spritenumber, voxelnumber
int aVoxelArray[MAXTILES];

extern int nextvoxid;

// Load all the voxel files using swvoxfil.txt script file
// Script file format:

//			# - Comment
//			spritenumber (in artfile), voxel number, filename
//			Ex. 1803 0 medkit2.kvx
//			    1804 1 shotgun.kvx
//				etc....

void LoadKVXFromScript(char *filename)
{
    int lNumber=0,lTile=0;  // lNumber is the voxel no. and lTile is the editart tile being
    // replaced.
    char *sName;            // KVS file being loaded in.

    int grabbed=0;          // Number of lines parsed

    sName = (char *)malloc(256);    // Up to 256 bytes for path
    ASSERT(sName != NULL);

    // zero out the array memory with -1's for pics not being voxelized
    memset(aVoxelArray,-1,sizeof(aVoxelArray));

    // Load the file
    if (!LoadScriptFile(filename)) return;

    do
    {
        GetToken(TRUE); // Crossing a line boundary on the end of line to first token
        // of a new line is permitted (and expected)
        if (endofscript)
            break;

        lTile = atol(token);

        GetToken(FALSE);
        lNumber = atol(token);

        GetToken(FALSE);
        strcpy(sName,token);            // Copy the whole token as a file name and path

        // Load the voxel file into memory
        if (!qloadkvx(lNumber,sName))
        {
            // Store the sprite and voxel numbers for later use
            aVoxelArray[lTile] = lNumber;   // Voxel num
        }

        if (lNumber >= nextvoxid)   // JBF: so voxels in the def file append to the list
            nextvoxid = lNumber + 1;

        grabbed++;
        ASSERT(grabbed < MAXSPRITES);

    }
    while (script_p < scriptend_p);

    free(scriptbuffer);
    script_p = NULL;
}


/// MISC ////////////////////////////////////////////////////////////////////

/*
extern int idleclock,slackerclock;

// Watch dog function.  Tracks user's work times.
void LogUserTime( SWBOOL bIsLoggingIn )
{
    int size, readsize;
    time_t time_of_day;
    char serialid[20],filename[100],fbase[20],buf[26],filetemp[100];
    FILE *fp;
    int tothours, totmins, totsecs, gtotalclock=0,gidleclock=0;
    ldiv_t mins_secs;
    ldiv_t hrs_mins;
    int i;

    char path[] = "o:\\user\\jimn\\logs\\";
//	char path[] = "c:\\jim\\sw\\";

    memset(filename,0,sizeof(filename));
    memset(fbase,0,sizeof(fbase));
    memset(serialid,0,sizeof(serialid));
    memset(buf,0,sizeof(buf));
    memset(filetemp,0,sizeof(filetemp));

    // Get the time of day user logged in to build
    time_of_day = time( NULL );

    // Get the serial number from the user's disk drive "it's unique!"
    system("dir > serid.bld");
    LoadScriptFile("serid.bld");

    // Go to the serial number
    for (i=0; i<11; i++)
    {
        GetToken (TRUE);
        if (endofscript)
            return;
    }

    // Copy the token to serialid
    strcpy(serialid,token);

    // Free the script memory when done
    free(scriptbuffer);
    script_p = NULL;

    // Build a file name using serial id.
    strcpy(filename,path);
    strncpy(fbase,serialid,8);
    strcat(fbase,".bld");
    strcat(filename,fbase);

    // Delete the temp file
    system("erase serid.bld");

    // Unhide the file so it can be opened
    _dos_setfileattr(filename,_A_NORMAL);


    // Open the file
    fp = fopen( filename, "a+" );

    // Opening on the network failed, try putting it on the current disk drive
    if(fp == NULL)
    {
        // Unhide the file so it can be opened/this works if file was created before!
        _dos_setfileattr(fbase,_A_NORMAL);
        fp = fopen( fbase, "a+" );
        strcpy(filetemp,fbase);
    } else
        strcpy(filetemp,filename);


    if( fp == NULL)
        return;
    else
    {
        if(bIsLoggingIn)
        {
            fprintf(fp, "//////////////////////////////\n");
            fprintf(fp, "User logged into build at: %s", _ctime( &time_of_day, buf ) );
        }else
        {
            totsecs = totalclock/120;	// Convert totalclock to seconds.

            mins_secs = ldiv( totsecs, 60L );
            totmins = mins_secs.quot;
            totsecs = mins_secs.rem;

            hrs_mins = ldiv( totmins, 60L);
            tothours = hrs_mins.quot;
            totmins = hrs_mins.rem;

            fprintf(fp, "TotalClock: %ld\n",totalclock);
#ifdef COMPUTE_TOTALS
            fprintf(fp, "IdleClock: %ld\n",slackerclock);
#endif
            fprintf(fp, "Time this session: %ld Hours %ld Mins %ld Secs\n",tothours,totmins,totsecs);
#ifdef COMPUTE_TOTALS
            totsecs = (totalclock-slackerclock)/120;	// Convert totalclock to seconds.
            if(totsecs<=0) totsecs = 0;

            mins_secs = ldiv( totsecs, 60L );
            totmins = mins_secs.quot;
            totsecs = mins_secs.rem;

            hrs_mins = ldiv( totmins, 60L);
            tothours = hrs_mins.quot;
            totmins = hrs_mins.rem;
            fprintf(fp, "Time - idleclock : %ld Hours %ld Mins %ld Secs\n",tothours,totmins,totsecs);
#endif
        }

        fclose( fp );
    }

#if 1
    if(!bIsLoggingIn)
    {
        // Compute total time for file
        LoadScriptFile(filetemp);

        do {
            GetToken (TRUE);

            if (endofscript)
                break;

            if(!strcmpi(token,"totalclock:"))
            {
                GetToken(TRUE);
                gtotalclock += atol(token);
            }
#if 0
            if(!strcmpi(token,"idleclock:"))
            {
                GetToken(TRUE);
                gidleclock += atol(token);
            }
#endif

        } while (script_p < scriptend_p);

        // Free the script memory when done
        free(scriptbuffer);
        script_p = NULL;

        // Open the file
        fp = fopen( filetemp, "a+" );

        // Now compute the grand total
        if(fp != NULL)
        {
            totsecs = gtotalclock/120;	// Convert totalclock to seconds.

            mins_secs = ldiv( totsecs, 60L );
            totmins = mins_secs.quot;
            totsecs = mins_secs.rem;

            hrs_mins = ldiv( totmins, 60L);
            tothours = hrs_mins.quot;
            totmins = hrs_mins.rem;

            fprintf(fp, "\nTotal time so far  : %ld Hours %ld Mins %ld Secs\n",tothours,totmins,totsecs);

#if 0
            totsecs = (gtotalclock-gidleclock)/120;	// Convert totalclock to seconds.
            if(totsecs<=0) totsecs = 0;

            mins_secs = ldiv( totsecs, 60L );
            totmins = mins_secs.quot;
            totsecs = mins_secs.rem;

            hrs_mins = ldiv( totmins, 60L);
            tothours = hrs_mins.quot;
            totmins = hrs_mins.rem;

            fprintf(fp, "\nTotal actual time  : %ld Hours %ld Mins %ld Secs\n",tothours,totmins,totsecs);
#endif

            fclose(fp);
        }
    }
#endif

    _dos_setfileattr(filename,_A_HIDDEN);


}
*/
