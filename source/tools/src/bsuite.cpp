/*
    Build Game Customization Suite
    Copyright (c) 1999, 2004 Jonathon Fowler

    15 September 2004

    This is the source code to BCS. It was written in Borland Turbo C++ for DOS
    and [was] a 16bit real-mode DOS application. I'm releasing the code because
    I have no reason to keep it a secret. Some folks might find it interesting.

    BTW, you can use this code for any purpose you want.

    Jonathon Fowler
    jf@jonof.id.au
    http://www.jonof.id.au/
*/
/*
    NOTE: This program does not fall under BUILDLIC.
*/
// DOS 16-bit real mode UI --> portable command line conversion by Hendricks266

#include "compat.h"

const char APP_NAME[] = "Build Game Customization Suite v0.2-EDuke32";
const char APP_CPRT[] = "Copyright (c) 1999, 2004 Jonathon Fowler";

int bsExtractD3DPalette(int, const char*);
int bsUpdateD3DPalette(int, const char*);

int bsExtractPalette(const char*);
int bsUpdatePalette(const char*);

const char *MainMenuStrings[] = {
        "Options:",
        "Extract Duke Nukem 3D-specific Palettes",
        "Update Duke Nukem 3D-specific Palettes",
        "Extract Build Game Palette",
        "Update Build Game Palette"
};

const char *D3DMenuStrings[] = {
        "Sub-Option: Duke Nukem 3D-specific Palettes:",
        "Water Palette",
        "Night-Vision Palette",
        "Title Screen Palette",
        "3D Realms Logo Palette",
        "Episode 1 Ending Animation Palette"
};

const char *pal_deffn[] = {
        "GAME.PAL",
        "D3DWATER.PAL",
        "D3DNVIS.PAL",
        "D3DTITLE.PAL",
        "D3D3DR.PAL",
        "D3DEP1.PAL"
    };

int main(const int32_t argc, const char **argv)
{
    int opt = 0, d3dpal = 0, k = 0;
    int16_t i = 1;
    char const * filename = NULL; // This is only a pointer. Do not strcpy to it.
    char const * c = NULL;

    Bprintf("%s\n%s\n\n", APP_NAME, APP_CPRT);

    if (argc > 1)
    {
        opt = Bstrtol(argv[i++],NULL,10);
        if ((opt == 1 || opt == 2) && i < argc) // Duke-specific palettes
            d3dpal = Bstrtol(argv[i++],NULL,10);

        while (i < argc)
        {
            c = (char const *)argv[i];
            if ((*c == '-')
#ifdef _WIN32
                    || (*c == '/')
#endif
               )
            {
                ++c;
                if (!Bstrcasecmp(c,"f"))
                {
                    if (argc > i+1)
                        filename = (char const *)argv[++i];
                    ++i;
                    continue;
                }

            }
            ++i;
        }
    }

    if (opt < 1 || opt > 4 || (opt < 3 && (d3dpal < 1 || d3dpal > 5)))
    {
        Bprintf("usage: %s <option> <sub-option(s)> [-f filename]\n",argv[0]);
        Bprintf("If a filename is not specified, internal defaults will be used.\n");
        Bprintf("\n");
        for (k = 0; k < 5; ++k)
        {
            if (k > 0)
                Bprintf("%d - ",k);
            Bprintf("%s\n",MainMenuStrings[k]);
        }
        Bprintf("\n");
        for (k = 0; k < 6; ++k)
        {
            if (k > 0)
                Bprintf("%d - ",k);
            Bprintf("%s\n",D3DMenuStrings[k]);
        }
        Bprintf("\n");
/*
        Bprintf(          "This program is, well, I guess freeware. Questions, suggestions,\n"
                         "comments and bug reports are welcome at jf@jonof.id.au. Visit\n"
                         "my website at http://www.jonof.id.au/ for more information\n"
                         "on this program and others that I may happen to write.\n\n");
*/
    }
    else
    {
        if (filename == NULL)
            switch (opt)
            {
                case 1: // Duke-specific palettes
                case 2:
                    filename = (char const *)pal_deffn[d3dpal];
                    break;

                case 3: // game palette
                case 4:
                    filename = (char const *)pal_deffn[0];
                    break;
            }

        switch (opt)
        {
            case 1: // extract Duke-specific palettes
                bsExtractD3DPalette(d3dpal-1, filename);
                break;

            case 2: // update Duke-specific palettes
                bsUpdateD3DPalette(d3dpal-1, filename);
                break;

            case 3: // extract game palette
                bsExtractPalette(filename);
                break;

            case 4: // update game palette
                bsUpdatePalette(filename);
                break;
        }
    }

    return 0;
}





const int pal_offsets[5] = {6426, 7194, 7962, 8730, 9498};


#define PAL_LEN            768

/////////////////////////////////////////////////
//
// Extract and update functions

int bsExtractD3DPalette(int palnum, const char *outfn)
{
    BFILE *lookup, *out;
    char cb[3];
    int lp;

    Bprintf("Using: %s\n",outfn);

    lookup = Bfopen("LOOKUP.DAT", "rb");
    if (lookup == NULL)
    {
        // could not open LOOKUP.DAT
        Bprintf("Error opening LOOKUP.DAT!\n"
                            "Make sure that the file is in the current\n"
                            "directory, then try again.\n\n");

        return 1;
    }

    // create output file
    out = Bfopen(outfn, "w+t");
    if (out == NULL)
    {
        Bfclose(lookup);
        Bprintf("Error creating output file!\n"
                            "The file may be open by another program\n"
                            "or is read-only, or the disk may be read-only.\n\n");
        return 1;
    }

    // write out the palette data in PSP format

    // find palette data
    Bfseek(lookup, pal_offsets[palnum], SEEK_SET);

    // write out a Paint Shop Pro palette file
    Bfprintf(out, "JASC-PAL\n0100\n256\n");
    for (lp=0; lp < 256; lp++)
    {
        cb[0] = Bfgetc(lookup) * 4;
        cb[1] = Bfgetc(lookup) * 4;
        cb[2] = Bfgetc(lookup) * 4;

        Bfprintf(out, "%d %d %d\n", cb[0], cb[1], cb[2]);
    }

    // close files
    Bfclose(out);
    Bfclose(lookup);

    Bprintf("Palette dumped successfully!\n\n");

    return 0;
}


int bsUpdateD3DPalette(int palnum, const char *palfn)
{
    BFILE *lookup, *pal;
    char cb[3], work[64];
    int lp;

    Bprintf("Using: %s\n",palfn);

    lookup = Bfopen("LOOKUP.DAT", "r+b");
    if (lookup == NULL)
    {
        // could not open LOOKUP.DAT
        Bprintf("Error opening LOOKUP.DAT!\n"
                            "The file may be open by another program\n"
                            "or is read-only, or the disk may be read-only.\n\n");

        return 1;
    }

    // open source file
    pal = Bfopen(palfn, "rt");
    if (pal == NULL)
    {
        Bfclose(lookup);
        Bprintf("Error opening palette file!\n"
                            "Make sure that the file exists.\n\n");
        return 1;
    }

    // read the palette data and write it to LOOKUP.DAT

    Bfgets(work, 64, pal);
    if (strncmp(work, "JASC-PAL", 8))
    {
        Bfclose(pal); Bfclose(lookup);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be in a format\n"
                            "other than Paint Shop Pro format. This program\n"
                            "works only with Paint Shop Pro palette files.\n\n");
        return 1;
    }

    Bfgets(work, 64, pal);
    if (strncmp(work, "0100", 4))
    {
        Bfclose(pal); Bfclose(lookup);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be in a version\n"
                            "other than 0100. This program works only with\n"
                            "Paint Shop Pro palettes of version 0100.\n\n");
        return 1;
    }

    Bfgets(work, 64, pal);
    if (strncmp(work, "256", 3))
    {
        Bfclose(pal); Bfclose(lookup);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be for a palette\n"
                            "size other than 256 colours. This program works\n"
                            "only with Paint Shop Pro palettes of 256 colours.\n\n");
        return 1;
    }

    // find palette data
    Bfseek(lookup, pal_offsets[palnum], SEEK_SET);

    // write out new palette info
    for (lp=0; lp < 256; lp++)
    {
        Bfscanf(pal, "%c %c %c\n", &cb[0], &cb[1], &cb[2]);

        Bfputc(cb[0] / 4, lookup);
        Bfputc(cb[1] / 4, lookup);
        Bfputc(cb[2] / 4, lookup);
    }

    // close files
    Bfclose(pal);
    Bfclose(lookup);

    Bprintf("Palette updated successfully!\n\n");

    return 0;
}




// Format of PALETTE.DAT files
//
//   256 bytes     - palette
//   2 bytes       - short: number of palette lookups
//   n*256 bytes   - shading lookup tables
//   256*256 bytes - translucency lookup array



int bsExtractPalette(const char *outfn)
{
    BFILE *palette, *out;
    char cb[3];
    int lp;

    Bprintf("Using: %s\n",outfn);

    palette = Bfopen("PALETTE.DAT", "rb");
    if (palette == NULL)
    {
        // could not open PALETTE.DAT
        Bprintf("Error opening PALETTE.DAT!\n"
                            "Make sure that the file is in the current\n"
                            "directory, then try again.\n\n");

        return 1;
    }

    // create output file
    out = Bfopen(outfn, "w+t");
    if (out == NULL)
    {
        Bfclose(palette);
        Bprintf("Error creating output file!\n"
                            "The file may be open by another program\n"
                            "or is read-only, or the disk may be read-only.\n\n");
        return 1;
    }

    // write out the palette data in PSP format

    // write out a Paint Shop Pro palette file
    Bfprintf(out, "JASC-PAL\n0100\n256\n");
    for (lp=0; lp < 256; lp++)
    {
        cb[0] = Bfgetc(palette) * 4;
        cb[1] = Bfgetc(palette) * 4;
        cb[2] = Bfgetc(palette) * 4;

        Bfprintf(out, "%d %d %d\n", cb[0], cb[1], cb[2]);
    }

    // close files
    Bfclose(out);
    Bfclose(palette);

    Bprintf("Palette dumped successfully!\n\n");

    return 0;
}


int bsUpdatePalette(const char *palfn)
{
    BFILE *palette, *pal;
    char cb[3], work[64];
    int lp;

    Bprintf("Using: %s\n",palfn);

    palette = Bfopen("PALETTE.DAT", "w+b");
    if (palette == NULL)
    {
        // could not open LOOKUP.DAT
        Bprintf("Error opening PALETTE.DAT!\n"
                            "The file may be open by another program\n"
                            "or is read-only, or the disk may be read-only.\n\n");

        return 1;
    }

    // open source file
    pal = Bfopen(palfn, "rt");
    if (pal == NULL)
    {
        Bfclose(palette);
        Bprintf("Error opening palette file!\n"
                            "Make sure that the file exists.\n\n");
        return 1;
    }

    // read the palette data and write it to PALETTE.DAT

    Bfgets(work, 64, pal);
    if (strncmp(work, "JASC-PAL", 8))
    {
        Bfclose(pal); Bfclose(palette);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be in a format\n"
                            "other than Paint Shop Pro format. This program\n"
                            "works only with Paint Shop Pro palette files.\n\n");
        return 1;
    }

    Bfgets(work, 64, pal);
    if (strncmp(work, "0100", 4))
    {
        Bfclose(pal); Bfclose(palette);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be in a version\n"
                            "other than 0100. This program works only with\n"
                            "Paint Shop Pro palettes of version 0100.\n\n");
        return 1;
    }

    Bfgets(work, 64, pal);
    if (strncmp(work, "256", 3))
    {
        Bfclose(pal); Bfclose(palette);
        Bprintf("Error validating palette file!\n"
                            "This palette file appears to be for a palette\n"
                            "size other than 256 colours. This program works\n"
                            "only with Paint Shop Pro palettes of 256 colours.\n\n");
        return 1;
    }

    // write out new palette info
    for (lp=0; lp < 256; lp++)
    {
        Bfscanf(pal, "%c %c %c\n", &cb[0], &cb[1], &cb[2]);

        // put the bytes into the basergb array as well
        Bfputc(cb[0] / 4, palette);
        Bfputc(cb[1] / 4, palette);
        Bfputc(cb[2] / 4, palette);
    }

    // close files
    Bfclose(pal);
    Bfclose(palette);

    Bprintf("Palette updated successfully!\n"
                        "Now run TRANSPAL.EXE to create the shading\n"
                        "and translucency tables for the palette.\n\n");

    return 0;
}
