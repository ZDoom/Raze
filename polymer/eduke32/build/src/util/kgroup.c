// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"

// Glibc doesn't provide this function, so for the sake of less ugliess
// for all platforms, here's a replacement just for this program.
static void jstrupr(char *s) { while (*s) { *s = Btoupper(*s); s++; } }

#define MAXFILES 4096

static char buf[65536];		// These limits should be abolished one day

static int numfiles;
static char filespec[MAXFILES][128], filelist[MAXFILES][16];
static int fileleng[MAXFILES];


static char *matchstr = "*.*";
int checkmatch(const struct Bdirent *a)
{
	if (a->mode & BS_IFDIR) return 0;	// is a directory
	if (a->namlen > 12) return 0;	// name too long
	return Bwildmatch(a->name, matchstr);
}

int filesize(const char *path, const char *name)
{
	char p[BMAX_PATH];
	struct stat st;

	strcpy(p, path);
	strcat(p, "/");
	strcat(p, name);

	if (!stat(p, &st)) return st.st_size;
	return 0;
}

void findfiles(const char *dafilespec)
{
	struct Bdirent *name;
	int daspeclen;
	char daspec[128], *dir;
	BDIR *di;

	strcpy(daspec,dafilespec);
	daspeclen=strlen(daspec);
	while ((daspec[daspeclen] != '\\') && (daspec[daspeclen] != '/') && (daspeclen > 0)) daspeclen--;
	if (daspeclen > 0) {
		daspec[daspeclen]=0;
		dir = daspec;
		matchstr = &daspec[daspeclen+1];
	} else {
		dir = ".";
		matchstr = daspec;
	}

	di = Bopendir(dir);
	if (!di) return;

	while ((name = Breaddir(di))) {
		if (!checkmatch(name)) continue;

		strcpy(&filelist[numfiles][0],name->name);
		jstrupr(&filelist[numfiles][0]);
		fileleng[numfiles] = name->size;
		filelist[numfiles][12] = (char)(fileleng[numfiles]&255);
		filelist[numfiles][13] = (char)((fileleng[numfiles]>>8)&255);
		filelist[numfiles][14] = (char)((fileleng[numfiles]>>16)&255);
		filelist[numfiles][15] = (char)((fileleng[numfiles]>>24)&255);

		strcpy(filespec[numfiles],dir);
		strcat(filespec[numfiles], "/");
		strcat(filespec[numfiles],name->name);

		numfiles++;
		if (numfiles > MAXFILES)
		{
			printf("FATAL ERROR: TOO MANY FILES SELECTED! (MAX is 4096)\n");
			exit(0);
		}
	}

	Bclosedir(di);
}

int main(int argc, char **argv)
{
	int i, j, k, l, fil, fil2;

	if (argc < 3)
	{
		printf("KGROUP [grouped file][@file or filespec...]           by Kenneth Silverman\n");
		printf("   This program collects many files into 1 big uncompressed file called a\n");
		printf("   group file\n");
		printf("   Ex: kgroup stuff.dat *.art *.map *.k?? palette.dat tables.dat\n");
		printf("      (stuff.dat is the group file, the rest are the files to add)\n");
		exit(0);
	}

	numfiles = 0;
	for(i=argc-1;i>1;i--)
	{
		if (argv[i][0] == '@')
		{
			if ((fil = Bopen(&argv[i][1],BO_BINARY|BO_RDONLY,BS_IREAD)) != -1)
			{
				l = Bread(fil,buf,65536);
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
				Bclose(fil);
			}
		}
		else
			findfiles(argv[i]);
	}

	if ((fil = Bopen(argv[1],BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
	{
		printf("Error: %s could not be opened\n",argv[1]);
		exit(0);
	}
	Bwrite(fil,"KenSilverman",12);
	Bwrite(fil,&numfiles,4);
	Bwrite(fil,filelist,numfiles<<4);

	for(i=0;i<numfiles;i++)
	{
		printf("Adding %s...\n",filespec[i]);
		if ((fil2 = Bopen(filespec[i],BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
		{
			printf("Error: %s not found\n",filespec[i]);
			Bclose(fil);
			return(0);
		}
		for(j=0;j<fileleng[i];j+=65536)
		{
			k = min(fileleng[i]-j,65536);
			Bread(fil2,buf,k);
			if (Bwrite(fil,buf,k) < k)
			{
				Bclose(fil2);
				Bclose(fil);
				printf("OUT OF HD SPACE!\n");
				return(0);
			}
		}
		Bclose(fil2);
	}
	Bclose(fil);
	printf("Saved to %s.\n",argv[1]);

	return 0;
}

