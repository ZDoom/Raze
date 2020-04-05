
#define LIBDIVIDE_BODY
#include "compat.h"
#include "build.h"
#include "scriptfile.h"

#include "baselayer.h"

#include "common.h"

#include "../../glbackend/glbackend.h"

// def/clipmap handling

#ifdef HAVE_CLIPSHAPE_FEATURE
TArray<FString> g_clipMapFiles;
#endif

void SetClipshapes()
{
#ifdef HAVE_CLIPSHAPE_FEATURE
	// pre-form the default 10 clipmaps
	for (int j = '0'; j <= '9'; ++j)
	{
		char clipshape[16] = "_clipshape0.map";

		clipshape[10] = j;
		g_clipMapFiles.Push(clipshape);
	}
#endif
}

//////////

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens)
{
    char *tok;
    int32_t i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=ntokens-1; i>=0; i--)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

//////////


// Copy FN to WBUF and append an extension if it's not there, which is checked
// case-insensitively.
// Returns: 1 if not all characters could be written to WBUF, 0 else.
int32_t maybe_append_ext(char *wbuf, int32_t wbufsiz, const char *fn, const char *ext)
{
    const int32_t slen=Bstrlen(fn), extslen=Bstrlen(ext);
    const int32_t haveext = (slen>=extslen && Bstrcasecmp(&fn[slen-extslen], ext)==0);

    Bassert((intptr_t)wbuf != (intptr_t)fn);  // no aliasing

    // If 'fn' has no extension suffixed, append one.
    return (snprintf(wbuf, wbufsiz, "%s%s", fn, haveext ? "" : ext) >= wbufsiz);
}


int32_t ldist(const void *s1, const void *s2)
{
    auto sp1 = (vec2_t const *)s1;
    auto sp2 = (vec2_t const *)s2;
    return sepldist(sp1->x - sp2->x, sp1->y - sp2->y)
        + (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 1 : 0);
}

int32_t dist(const void *s1, const void *s2)
{
    auto sp1 = (vec3_t const *)s1;
    auto sp2 = (vec3_t const *)s2;
    return sepdist(sp1->x - sp2->x, sp1->y - sp2->y, sp1->z - sp2->z);
}

int32_t FindDistance2D(int32_t x, int32_t y)
{
    return sepldist(x, y);
}

int32_t FindDistance3D(int32_t x, int32_t y, int32_t z)
{
    return sepdist(x, y, z);
}
