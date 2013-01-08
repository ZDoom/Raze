/*
 * High-colour textures support for Polymost
 * by Jonathon Fowler
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#ifdef USE_OPENGL

#include "build.h"
#include "compat.h"
#include "kplib.h"
#include "hightile.h"
#include "baselayer.h"


palette_t hictinting[MAXPALOOKUPS];

hicreplctyp *hicreplc[MAXTILES];
char hicfirstinit = 0;

//
// find the index into hicreplc[] which contains the replacement tile particulars
//
hicreplctyp *hicfindsubst(int32_t picnum, int32_t palnum, int32_t skybox)
{
    if (!hicfirstinit || (uint32_t)picnum >= (uint32_t)MAXTILES) return NULL;

    do
    {
        if (skybox)
        {
            hicreplctyp *hr = hicreplc[picnum];
            for (; hr; hr = hr->next)
                if (hr->palnum == palnum && hr->skybox && !hr->skybox->ignore)
                    return hr;
        }
        else
        {
            hicreplctyp *hr = hicreplc[picnum];
            for (; hr; hr = hr->next)
                if (hr->palnum == palnum && !hr->ignore)
                    return hr;
        }

        if (!palnum || palnum >= (MAXPALOOKUPS - RESERVEDPALS)) break;
        palnum = 0;
    }
    while (1);

    return NULL;	// no replacement found
}


//
// hicinit()
//   Initialize the high-colour stuff to default.
//
void hicinit(void)
{
    int32_t i,j;
    hicreplctyp *hr, *next;

    for (i=0; i<MAXPALOOKUPS; i++)  	// all tints should be 100%
    {
        hictinting[i].r = hictinting[i].g = hictinting[i].b = 0xff;
        hictinting[i].f = 0;
    }

    if (hicfirstinit)
        for (i=MAXTILES-1; i>=0; i--)
        {
            for (hr=hicreplc[i]; hr;)
            {
                next = hr->next;

                if (hr->skybox)
                {
                    for (j=5; j>=0; j--)
                        if (hr->skybox->face[j])
                            Bfree(hr->skybox->face[j]);
                    Bfree(hr->skybox);
                }

                if (hr->filename)
                    Bfree(hr->filename);

                Bfree(hr);

                hr = next;
            }
        }

    Bmemset(hicreplc,0,sizeof(hicreplc));

    hicfirstinit = 1;
}


//
// hicsetpalettetint(pal,r,g,b,effect)
//   The tinting values represent a mechanism for emulating the effect of global sector
//   palette shifts on true-colour textures and only true-colour textures.
//   effect bitset: 1 = greyscale, 2 = invert
//
void hicsetpalettetint(int32_t palnum, char r, char g, char b, char effect)
{
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return;
    if (!hicfirstinit) hicinit();

    hictinting[palnum].r = r;
    hictinting[palnum].g = g;
    hictinting[palnum].b = b;
    hictinting[palnum].f = effect & HICEFFECTMASK;
}


//
// hicsetsubsttex(picnum,pal,filen,alphacut)
//   Specifies a replacement graphic file for an ART tile.
//
int32_t hicsetsubsttex(int32_t picnum, int32_t palnum, const char *filen, float alphacut, float xscale, float yscale, float specpower, float specfactor, char flags)
{
    hicreplctyp *hr, *hrn;

    if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;
    if (!hicfirstinit) hicinit();

    for (hr = hicreplc[picnum]; hr; hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr)
    {
        // no replacement yet defined
        hrn = (hicreplctyp *)Bcalloc(1,sizeof(hicreplctyp));
        if (!hrn) return -1;
        hrn->palnum = palnum;
    }
    else hrn = hr;

    // store into hicreplc the details for this replacement
    if (hrn->filename) Bfree(hrn->filename);

    hrn->filename = Bstrdup(filen);
    if (!hrn->filename)
    {
        if (hrn->skybox) return -1;	// don't free the base structure if there's a skybox defined
        if (hr == NULL) Bfree(hrn);	// not yet a link in the chain
        return -1;
    }
    hrn->ignore = 0;
    hrn->alphacut = min(alphacut,1.0);
    hrn->xscale = xscale;
    hrn->yscale = yscale;
    hrn->specpower = specpower;
    hrn->specfactor = specfactor;
    hrn->flags = flags;
    if (hr == NULL)
    {
        hrn->next = hicreplc[picnum];
        hicreplc[picnum] = hrn;
    }

    if (tilesizx[picnum]<=0 || tilesizy[picnum]<=0)
    {
        static int32_t first=1;
        if (first)
        {
            initprintf("Warning: defined hightile replacement for empty tile %d.", picnum);
            initprintf(" Maybe some tilesXXX.art are not loaded?");
            initprintf("\n");
            first = 0;
        }
    }

    //printf("Replacement [%d,%d]: %s\n", picnum, palnum, hicreplc[i]->filename);

    return 0;
}


//
// hicsetskybox(picnum,pal,faces[6])
//   Specifies a graphic files making up a skybox.
//
int32_t hicsetskybox(int32_t picnum, int32_t palnum, char *faces[6])
{
    hicreplctyp *hr, *hrn;
    int32_t j;

    if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;
    for (j=5; j>=0; j--) if (!faces[j]) return -1;
    if (!hicfirstinit) hicinit();

    for (hr = hicreplc[picnum]; hr; hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr)
    {
        // no replacement yet defined
        hrn = (hicreplctyp *)Bcalloc(1,sizeof(hicreplctyp));
        if (!hrn) return -1;

        hrn->palnum = palnum;
    }
    else hrn = hr;

    if (!hrn->skybox)
    {
        hrn->skybox = (struct hicskybox_t *)Bcalloc(1,sizeof(struct hicskybox_t));
        if (!hrn->skybox)
        {
            if (hr == NULL) Bfree(hrn);	// not yet a link in the chain
            return -1;
        }
    }
    else
    {
        for (j=5; j>=0; j--)
        {
            if (hrn->skybox->face[j])
                Bfree(hrn->skybox->face[j]);
        }
    }

    // store each face's filename
    for (j=0; j<6; j++)
    {
        hrn->skybox->face[j] = Bstrdup(faces[j]);
        if (!hrn->skybox->face[j])
        {
            for (--j; j>=0; --j)	// free any previous faces
                Bfree(hrn->skybox->face[j]);
            Bfree(hrn->skybox);
            hrn->skybox = NULL;
            if (hr == NULL) Bfree(hrn);
            return -1;
        }
    }
    hrn->skybox->ignore = 0;
    if (hr == NULL)
    {
        hrn->next = hicreplc[picnum];
        hicreplc[picnum] = hrn;
    }

    return 0;
}


//
// hicclearsubst(picnum,pal)
//   Clears a replacement for an ART tile, including skybox faces.
//
int32_t hicclearsubst(int32_t picnum, int32_t palnum)
{
    hicreplctyp *hr, *hrn = NULL;

    if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;
    if (!hicfirstinit) return 0;

    for (hr = hicreplc[picnum]; hr; hrn = hr, hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr) return 0;

    if (hr->filename) Bfree(hr->filename);
    if (hr->skybox)
    {
        int32_t i;
        for (i=5; i>=0; i--)
            if (hr->skybox->face[i])
                Bfree(hr->skybox->face[i]);
        Bfree(hr->skybox);
    }

    if (hrn) hrn->next = hr->next;
    else hicreplc[picnum] = hr->next;
    Bfree(hr);

    return 0;
}

#else /* USE_OPENGL */

#include "compat.h"

void hicsetpalettetint(int32_t palnum, char r, char g, char b, char effect)
{
    UNREFERENCED_PARAMETER(palnum);
    UNREFERENCED_PARAMETER(r);
    UNREFERENCED_PARAMETER(g);
    UNREFERENCED_PARAMETER(b);
    UNREFERENCED_PARAMETER(effect);
}
int32_t hicsetsubsttex(int32_t picnum, int32_t palnum, const char *filen, float alphacut)
{
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(palnum);
    UNREFERENCED_PARAMETER(filen);
    UNREFERENCED_PARAMETER(alphacut);
    return 0;
}
int32_t hicsetskybox(int32_t picnum, int32_t palnum, char *faces[6])
{
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(palnum);
    UNREFERENCED_PARAMETER(faces);
    return 0;
}
int32_t hicclearsubst(int32_t picnum, int32_t palnum)
{
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(palnum);
    return 0;
}

#endif

