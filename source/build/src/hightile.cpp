/*
 * High-colour textures support for Polymost
 * by Jonathon Fowler
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#include "build.h"

#ifdef USE_OPENGL

#include "compat.h"
#include "kplib.h"
#include "hightile.h"
#include "baselayer.h"


polytint_t hictinting[MAXPALOOKUPS];

hicreplctyp *hicreplc[MAXTILES];
int32_t hicinitcounter = 0;

//
// find the index into hicreplc[] which contains the replacement tile particulars
//
hicreplctyp *hicfindsubst(int picnum, int palnum, int nozero)
{
    if (!hicreplc[picnum] || !hicinitcounter) return NULL;

    do
    {
        hicreplctyp *hr = hicreplc[picnum];
        for (; hr; hr = hr->next)
            if (hr->palnum == palnum)
                return hr;

        if (!palnum || nozero)
            return NULL;

        palnum = 0;
    } while (1);

    return NULL;	// no replacement found
}

//
// this is separate because it's not worth passing an extra parameter which is "0" in 99.9999% of cases
// to the regular hicfindsubst() function
//
hicreplctyp *hicfindskybox(int picnum, int palnum, int nozero)
{
    if (!hicreplc[picnum] || !hicinitcounter) return NULL;

    do
    {
        hicreplctyp *hr = hicreplc[picnum];
        for (; hr; hr = hr->next)
            if (hr->skybox && hr->palnum == palnum)
                return hr;

        if (!palnum || nozero)
            return NULL;

        palnum = 0;
    } while (1);

    return NULL;	// no replacement found
}


//
// hicinit()
//   Initialize the high-colour stuff to default.
//
void hicinit(void)
{
    int32_t i;

    for (i=0; i<MAXPALOOKUPS; i++)  	// all tints should be 100%
    {
        polytint_t & tint = hictinting[i];
        tint.r = tint.g = tint.b = 0xff;
        tint.f = 0;
    }

    if (hicinitcounter)
    {
        hicreplctyp *hr, *next;
        int32_t j;

        for (i=MAXTILES-1; i>=0; i--)
        {
            for (hr=hicreplc[i]; hr;)
            {
                next = hr->next;

                if (hr->skybox)
                {
                    for (j=5; j>=0; j--)
                        Bfree(hr->skybox->face[j]);
                    Bfree(hr->skybox);
                }

                Bfree(hr->filename);
                Bfree(hr);

                hr = next;
            }
        }
    }

    Bmemset(hicreplc,0,sizeof(hicreplc));

    hicinitcounter++;
}


//
// hicsetpalettetint(pal,r,g,b,sr,sg,sb,effect)
//   The tinting values represent a mechanism for emulating the effect of global sector
//   palette shifts on true-colour textures and only true-colour textures.
//   effect bitset: 1 = greyscale, 2 = invert
//
void hicsetpalettetint(int32_t palnum, char r, char g, char b, char sr, char sg, char sb, polytintflags_t effect)
{
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return;
    if (!hicinitcounter) hicinit();

    polytint_t & tint = hictinting[palnum];
    tint.r = r;
    tint.g = g;
    tint.b = b;
    tint.sr = sr;
    tint.sg = sg;
    tint.sb = sb;
    tint.f = effect;
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
    if (!hicinitcounter) hicinit();

    for (hr = hicreplc[picnum]; hr; hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr)
    {
        // no replacement yet defined
        hrn = (hicreplctyp *)Xcalloc(1,sizeof(hicreplctyp));
        hrn->palnum = palnum;
    }
    else hrn = hr;

    // store into hicreplc the details for this replacement
    Bfree(hrn->filename);

    hrn->filename = Xstrdup(filen);
    hrn->alphacut = min(alphacut,1.f);
    hrn->scale.x = xscale;
    hrn->scale.y = yscale;
    hrn->specpower = specpower;
    hrn->specfactor = specfactor;
    hrn->flags = flags;
    if (hr == NULL)
    {
        hrn->next = hicreplc[picnum];
        hicreplc[picnum] = hrn;
    }

    if (tilesiz[picnum].x<=0 || tilesiz[picnum].y<=0)
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
int32_t hicsetskybox( int32_t picnum, int32_t palnum, char *faces[6], int32_t flags )
{
    hicreplctyp *hr, *hrn;
    int32_t j;

    if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;
    for (j=5; j>=0; j--) if (!faces[j]) return -1;
    if (!hicinitcounter) hicinit();

    for (hr = hicreplc[picnum]; hr; hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr)
    {
        // no replacement yet defined
        hrn = (hicreplctyp *)Xcalloc(1,sizeof(hicreplctyp));
        hrn->palnum = palnum;
    }
    else hrn = hr;

    if (!hrn->skybox)
        hrn->skybox = (struct hicskybox_t *)Xcalloc(1,sizeof(struct hicskybox_t));
    else
    {
        for (j=0; j<6; j++)
            DO_FREE_AND_NULL(hrn->skybox->face[j]);
    }

    // store each face's filename
    for (j=0; j<6; j++)
        hrn->skybox->face[j] = Xstrdup(faces[j]);

    hrn->flags = flags;

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
    if (!hicinitcounter) return 0;

    for (hr = hicreplc[picnum]; hr; hrn = hr, hr = hr->next)
    {
        if (hr->palnum == palnum)
            break;
    }

    if (!hr) return 0;

    Bfree(hr->filename);
    if (hr->skybox)
    {
        int32_t i;
        for (i=5; i>=0; i--)
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

void hicsetpalettetint(int32_t palnum, char r, char g, char b, char sr, char sg, char sb, polytintflags_t effect)
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
int32_t hicsetskybox(int32_t picnum, int32_t palnum, char *faces[6], int32_t flags)
{
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(palnum);
    UNREFERENCED_PARAMETER(faces);
    UNREFERENCED_PARAMETER(flags);
    return 0;
}
int32_t hicclearsubst(int32_t picnum, int32_t palnum)
{
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(palnum);
    return 0;
}

#endif

