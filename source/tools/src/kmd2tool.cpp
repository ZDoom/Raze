
#include "compat.h"

typedef struct { float x, y, z; } point3d;

typedef struct
{  int id, vers, skinxsiz, skinysiz, framebytes; // id:"IPD2", vers:8
    int numskins, numverts, numuv, numtris, numglcmds, numframes;
    int ofsskins, ofsuv, ofstris, ofsframes, ofsglcmds, ofseof; // ofsskins: skin names (64 bytes each)
} md2typ;

typedef struct { point3d mul, add; } frametyp;

int main (const int argc, const char **argv)
{
    BFILE *fil;
    int i, leng;
    char *fbuf;
    float zoffset = 0.0f;
    md2typ *head;
    frametyp *fptr;

    if (argc != 4)
    {
        Bputs("KMD2TOOL <MD2 in file> <MD2 out file> <z offset>              by Ken Silverman");
        return(1);
    }
    if (!Bstrcasecmp(argv[1],argv[2]))
    {
        Bputs("Error: input and output filenames cannot be the same");
        return(2);
    }

    zoffset = Batof(argv[3]);
    if (0.0f == zoffset)
    {
        Bputs("Error: offset of zero");
        return(3);
    }

    fil = Bfopen(argv[1],"rb");
    if (!fil)
    {
        Bputs("Error: could not open input MD2");
        return(4);
    }

    Bfseek(fil, 0, SEEK_END);
    leng = Bftell(fil);
    Bfseek(fil, 0, SEEK_SET);

    fbuf = (char *)Bmalloc(leng * sizeof(char));
    if (!fbuf)
    {
        Bputs("Error: Could not allocate buffer");
        return(5);
    }

    Bfread(fbuf,leng,1,fil);
    Bfclose(fil);

    head = (md2typ *)fbuf;
    if ((head->id != 0x32504449) && (head->vers != 8)) // "IDP2"
    {
        Bfree(fbuf);
        Bputs("Error: input is not an MD2 file");
        return(6);
    }

    for(i=0; i<head->numframes; ++i)
    {
        fptr = (frametyp *)&fbuf[head->ofsframes+head->framebytes*i];
        Bprintf("frame %2d scale:%f,%f,%f offs:%f,%f,%f\n",i,fptr->mul.x,fptr->mul.y,fptr->mul.z,fptr->add.x,fptr->add.y,fptr->add.z);
        fptr->add.z += zoffset;
    }

    fil = Bfopen(argv[2],"wb");
    if (!fil)
    {
        Bputs("Error: could not open output file for writing");
        return(7);
    }
    Bfwrite(fbuf,leng,1,fil);
    Bfclose(fil);

    Bfree(fbuf);

    return(0);
}
