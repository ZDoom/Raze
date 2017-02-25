/*
    .SSI File Unpacker
    Copyright (c) 2003 Jonathon Fowler

    This is a small program to extract the files from the .SSI package format
    which Sunstorm Interactive expansion packs for games like Duke Nukem 3D
    are distributed in. It is unsupported but should errors arise, bug reports
    are welcome.

    Update: 12 June 2003

    This updated version includes the ability to extract the SSI revision 2
    format as used in Duke Carribean.


    This program is distributed under the terms of the GNU General Public
    License Version 2 which can be found in the included GNU.txt file.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


    Jonathon Fowler
    jf@jonof.id.au
    http://www.jonof.id.au/
*/
/*
    NOTE: This program does not fall under BUILDLIC.
*/

#include "compat.h"

int main(int argc, char **argv)
{
    FILE *fp, *ofp;
    int32_t i,j=0;
    int32_t version;
    int32_t numfiles;
    unsigned char numchars;
    char title[33];
    char description[3][71];
    struct file {
        char name[13];
        int32_t length;
    } *filenames;
    char runfile[13] = "<unknown>";
    char buf[1024];
    int32_t mode=0, param;

    puts("unpackssi - .SSI File Unpacker\n"
         "Copyright (c) 2003 Jonathon Fowler\n"
         "This software is distributed under the terms of the GNU General Public License.");

    if (argc<2) {
        puts("\nUsage: unpackssi [-l] <ssifile.ssi> [ssifile2.ssi ...]");
        puts("  Unpacks the contents of an SSI file (like those which Sunstorm Interactive");
        puts("  expansion packs for Duke Nukem 3D are distributed in) to the current");
        puts("  directory. NOTE: Any files which already exist and have the same name as the");
        puts("  files contained in the SSI file will be overwritten when extracting.");
        puts("\nSwitches:");
        puts("  -l  List files (no extraction)\n");
        return -1;
    } else {
        param = 1;
        if (argv[1][0] == '-') {
            param++;
            switch (argv[1][1]) {
                case 'l': mode = 1; break;
                default: printf("Unrecognised switch: %c.\n", argv[1][1]); break;
            }
        }
    }

    while (param < argc) {
        puts("");

        fp = fopen(argv[param],"rb");
        if (!fp) return -2;

        fread(&version, 4, 1, fp);
        if (version != 1 && version != 2) {
            fclose(fp);
            puts("Error: Unrecognized SSI version.");
            return -1;
        }

        printf("File is SSI Version %i\n", version);

        fread(&numfiles, 4, 1, fp);

        fread(&numchars, 1, 1, fp);
        if (numchars > 32) numchars = 32;
        fread(title, 32, 1, fp);
        title[numchars] = 0;

        if (version == 2) {
            fread(&numchars, 1, 1, fp);
            if (numchars > 12) numchars = 12;
            fread(runfile, 12, 1, fp);
            runfile[numchars] = 0;
        }

        for (i=0;i<3;i++) {
            fread(&numchars, 1, 1, fp);
            if (numchars > 70) numchars = 70;
            fread(description[i], 70, 1, fp);
            description[i][numchars] = 0;
        }

        filenames = (struct file *)malloc(sizeof(struct file) * numfiles);
        if (!filenames) {
            fclose(fp);
            puts("Error: Failed allocating memory for file index.");
            return -2;
        }

        for (i=0;i<numfiles;i++) {
            fread(&numchars, 1, 1, fp);
            if (numchars > 12) numchars = 12;
            fread(filenames[i].name, 12, 1, fp);
            filenames[i].name[numchars] = 0;

            fread(&filenames[i].length, 4, 1, fp);

            // seek past some stuff I can't seem to fully decipher at the moment
            fseek(fp, 34+1+69, SEEK_CUR);
        }

        printf("File:           %s\n"
               "Package Title:  %s\n"
               "Description:    %s\n"
               "                %s\n"
               "                %s\n"
               "Run Filename:   %s\n\n"
               , argv[param], title, description[0], description[1], description[2], runfile);

        if (mode == 1) {
            j=0;
            puts("File listing:");
        }

        for (i=0;i<numfiles;i++) {
            if (mode == 0) {
                ofp = fopen(filenames[i].name, "wb");
                if (!ofp) {
                    printf("Error: Failed creating %s. Unpack operation cancelled.\n", filenames[i].name);
                    break;
                }

                printf("Unpacking %s (%i bytes)...", filenames[i].name, filenames[i].length);

                for (j=filenames[i].length; j>1024; j-=1024) {
                    fread(buf, 1024, 1, fp);
                    fwrite(buf, 1024, 1, ofp);
                }
                if (j) {
                    fread(buf, j, 1, fp);
                    fwrite(buf, j, 1, ofp);
                }

                fclose(ofp);
                puts("done");
            } else if (mode == 1) {
                printf(" %-12s   %i bytes\n", filenames[i].name, filenames[i].length);
                j += filenames[i].length;
            }
        }

        if (mode == 1) {
            puts("");
            printf(" %i files, %i bytes\n", numfiles, j);
        }

        fclose(fp);
        free(filenames);

        param++;
    }

    return 0;
}
