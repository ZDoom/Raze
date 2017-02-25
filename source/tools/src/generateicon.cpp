
#include "compat.h"
#include "kplib.h"

struct icon {
    int32_t width,height;
    intptr_t *pixels;
    unsigned char *mask;
};

int writeicon(FILE *fp, struct icon *ico)
{
    int i;

    Bfprintf(fp,
        "#include \"sdlayer.h\"\n"
        "\n"
    );
    Bfprintf(fp,"static unsigned int sdlappicon_pixels[] = {\n");
    for (i=0;i<ico->width*ico->height;i++) {
        if ((i%6) == 0) Bfprintf(fp,"\t");
        else Bfprintf(fp," ");
        Bfprintf(fp, "0x%08lx,", (long)B_LITTLE32(ico->pixels[i]));
        if ((i%6) == 5) Bfprintf(fp,"\n");
    }
    if ((i%16) > 0) Bfprintf(fp, "\n");
    Bfprintf(fp, "};\n\n");

    Bfprintf(fp,"static unsigned char sdlappicon_mask[] = {\n");
    for (i=0;i<((ico->width+7)/8)*ico->height;i++) {
        if ((i%14) == 0) Bfprintf(fp,"\t");
        else Bfprintf(fp," ");
        Bfprintf(fp, "%3d,", ico->mask[i]);
        if ((i%14) == 13) Bfprintf(fp,"\n");
    }
    if ((i%16) > 0) Bfprintf(fp, "\n");
    Bfprintf(fp, "};\n\n");

    Bfprintf(fp,
        "struct sdlappicon sdlappicon = {\n"
        "    %d,%d,    // width,height\n"
        "    sdlappicon_pixels,\n"
        "    sdlappicon_mask\n"
        "};\n",
        ico->width, ico->height
    );

    return 0;
}

int main(int argc, char **argv)
{
    struct icon icon;
    int i;
    unsigned char *maskp, bm, *pp;

    if (argc<2) {
        Bfprintf(stderr, "generateicon <picture file>\n");
        return 1;
    }

    memset(&icon, 0, sizeof(icon));

    kpzload(argv[1], (intptr_t*)&icon.pixels, &icon.width, &icon.height);
    if (!icon.pixels) {
        Bfprintf(stderr, "Failure loading %s\n", argv[1]);
        return 1;
    }

    icon.mask = (unsigned char *)Bcalloc(icon.height, (icon.width+7)/8);
    if (!icon.mask) {
        Bfprintf(stderr, "Out of memory\n");
        Bfree(icon.pixels);
        return 1;
    }

    maskp = icon.mask;
    bm = 1;
    pp = (unsigned char *)icon.pixels;
    for (i=0; i<icon.height*icon.width; i++) {
        if (bm == 0) {
            bm = 1;
            maskp++;
        }

        {
            unsigned char c = pp[0];
            pp[0] = pp[2];
            pp[2] = c;
        }
        if (pp[3] > 0) *maskp |= bm;

        bm <<= 1;
        pp += 4;
    }

    writeicon(stdout, &icon);

    Bfree(icon.pixels);
    Bfree(icon.mask);

    return 0;
}
