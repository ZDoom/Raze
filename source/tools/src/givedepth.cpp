#include "compat.h"

#define MAXNUMTILES 256

int artversion, numtiles;
int localtilestart, localtileend;
short tilesizx[MAXNUMTILES], tilesizy[MAXNUMTILES];
int picanm[MAXNUMTILES];

FILE * openartfile(char *fn)
{
    FILE *fh;

    fh = fopen(fn,"rb");
    if (!fh) return NULL;

    fread(&artversion,4,1,fh); if (artversion != 1) { puts("Bad art version"); goto fail; }
    fread(&numtiles,4,1,fh);
    fread(&localtilestart,4,1,fh);
    fread(&localtileend,4,1,fh);
    numtiles = localtileend-localtilestart+1;
    if (numtiles > MAXNUMTILES) { puts("Too many tiles"); goto fail; }
    fread(tilesizx,2,numtiles,fh);
    fread(tilesizy,2,numtiles,fh);
    fread(picanm,4,numtiles,fh);

    return fh;
fail:
    fclose(fh);
    return NULL;
}

int main(int argc, char **argv)
{
    char const * palfile = "palette.dat", * voxfile = "output.vox";
    int tilenum;
    int depth;
    FILE *artfh, *voxfh, *palfh;
    int tilesz;
    unsigned char palette[768];
    unsigned char *tiledata;
    int i;

    if (argc < 4) {
        puts("givedepth <artfile.art> <tilenum> <depth> [palette.dat] [output.vox]");
        return 0;
    }

    tilenum = atoi(argv[2]);
    depth = atoi(argv[3]);
    if (argc >= 4) palfile = argv[4];
    if (argc >= 5) voxfile = argv[5];

    palfh = fopen(palfile,"rb");
    if (!palfh) {
        puts("Failure opening palette file");
        return 1;
    }
    fread(palette,768,1,palfh);
    fclose(palfh);

    artfh = openartfile(argv[1]);
    if (!artfh) {
        puts("Failure opening art file");
        return 1;
    }

    if (tilenum < 0 || tilenum > numtiles) {
        puts("Tilenum out of range in art file");
        fclose(artfh);
        return 1;
    }
    for (i=0; i<tilenum; i++) fseek(artfh, tilesizx[i] * tilesizy[i], SEEK_CUR);

    tilesz = tilesizx[tilenum]*tilesizy[tilenum];
    tiledata = (unsigned char *)malloc(tilesz);
    if (!tiledata) {
        puts("Could not allocate memory for tile");
        fclose(artfh);
        return 1;
    }

    fread(tiledata, tilesz, 1, artfh);
    fclose(artfh);

    voxfh = fopen(voxfile,"wb");
    if (!voxfh) {
        puts("Could not create output file");
        free(tiledata);
        return 1;
    }
    fwrite(&depth,4,1,voxfh);
    fwrite(&tilesizx[tilenum],4,1,voxfh);
    fwrite(&tilesizy[tilenum],4,1,voxfh);
    for (i=0; i<depth; i++) {
        fwrite(tiledata,tilesz,1,voxfh);
    }
    fwrite(palette,768,1,voxfh);

    free(tiledata);

    return 0;
}
