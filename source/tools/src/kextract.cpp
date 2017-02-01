// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "kplib.h"

#include <utime.h>

#define MAXFILES 4096

static char buf[65536];

static int numfiles, anyfiles4extraction;
static char marked4extraction[MAXFILES];
static char filelist[MAXFILES][16];
static int fileoffs[MAXFILES+1], fileleng[MAXFILES];

void findfiles(const char *dafilespec)
{
    char t[13];
    int i;

    for(i=numfiles-1;i>=0;i--)
    {
        memcpy(t,filelist[i],12);
        t[12] = 0;

        if (Bwildmatch(t,dafilespec)) {
            marked4extraction[i] = 1;
            anyfiles4extraction = 1;
        }
    }
}

int main(int argc, char **argv)
{
    int i, j, k, l, fil, fil2;
    struct Bstat stbuf;

    int onlylist = (argc==2);

    if (argc < 2)
    {
        Bprintf("KEXTRACT <groupfile.grp> [@file or filespec...]           by Kenneth Silverman\n");
        Bprintf("   This program extracts files from a previously grouped group file.\n");
        Bprintf("   You can extract files using the ? and * wildcards.\n");
        Bprintf("   Ex: kextract stuff.dat tiles000.art nukeland.map palette.dat\n");
        Bprintf("         (stuff.dat is the group file, the rest are the files to extract)\n");
        Bprintf("       kextract stuff.grp\n");
        Bprintf("         (simply lists the contents of stuff.grp)\n");
        return(0);
    }

    if ((fil = Bopen(argv[1],BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
    {
        Bprintf("Error: %s could not be opened\n",argv[1]);
        return(0);
    }

    Bread(fil,buf,16);
    if ((buf[0] != 'K') || (buf[1] != 'e') || (buf[2] != 'n') ||
         (buf[3] != 'S') || (buf[4] != 'i') || (buf[5] != 'l') ||
         (buf[6] != 'v') || (buf[7] != 'e') || (buf[8] != 'r') ||
         (buf[9] != 'm') || (buf[10] != 'a') || (buf[11] != 'n'))
    {
        Bclose(fil);
        Bprintf("Error: %s not a valid group file\n",argv[1]);
        return(0);
    }
    numfiles = *((int*)&buf[12]); numfiles = B_LITTLE32(numfiles);

    Bread(fil,filelist,numfiles<<4);

    j = 0;
    for(i=0;i<numfiles;i++)
    {
        k = *((int*)&filelist[i][12]); k = B_LITTLE32(k);
        filelist[i][12] = 0;
        fileoffs[i] = j;
        j += k;
    }
    fileoffs[numfiles] = j;

    if (onlylist)
    {
        for (i=0; i<numfiles; i++)
            Bprintf("%s\t\t%d\n", filelist[i], fileoffs[i+1]-fileoffs[i]);

        return 0;
    }

    for(i=0;i<numfiles;i++) marked4extraction[i] = 0;

    anyfiles4extraction = 0;
    for(i=argc-1;i>1;i--)
    {
        if (argv[i][0] == '@')
        {
            if ((fil2 = Bopen(&argv[i][1],BO_BINARY|BO_RDONLY,BS_IREAD)) != -1)
            {
                l = Bread(fil2,buf,65536);
                j = 0;
                while ((j < l) && (buf[j] <= 32)) j++;
                while (j < l)
                {
                    k = j;
                    while ((k < l) && (buf[k] > 32)) k++;

                    buf[k] = 0;
                    findfiles(&buf[j]);
                    j = k+1;

                    while ((j < l) && (buf[j] <= 32)) j++;
                }
                Bclose(fil2);
            }
        }
        else
            findfiles(argv[i]);
    }

    if (anyfiles4extraction == 0)
    {
        Bclose(fil);
        Bprintf("No files found in group file with those names\n");
        return(0);
    }

    if (Bfstat(fil, &stbuf) == -1)
        stbuf.st_mtime = 0;

    for(i=0;i<numfiles;i++)
    {
        if (marked4extraction[i] == 0) continue;

        fileleng[i] = fileoffs[i+1]-fileoffs[i];

        if ((fil2 = Bopen(filelist[i],BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
        {
            Bprintf("Error: Could not write to %s\n",filelist[i]);
            continue;
        }
        Bprintf("Extracting %s...\n",filelist[i]);
        Blseek(fil,fileoffs[i]+((numfiles+1)<<4),SEEK_SET);
        for(j=0;j<fileleng[i];j+=65536)
        {
            k = min(fileleng[i]-j,65536);
            Bread(fil,buf,k);
            if (Bwrite(fil2,buf,k) < k)
            {
                Bprintf("Write error (drive full?)\n");
                Bclose(fil2);
                Bclose(fil);
                return(0);
            }
        }
        Bclose(fil2);

        if (stbuf.st_mtime != 0)
        {
            struct utimbuf times;

            times.modtime = stbuf.st_mtime;
            times.actime = Btime();

            Butime(filelist[i],&times);
        }
    }
    Bclose(fil);

    return 0;
}

