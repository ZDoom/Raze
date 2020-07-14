
#define LIBDIVIDE_BODY
#include "compat.h"
#include "build.h"
#include "scriptfile.h"

#include "baselayer.h"

#include "common.h"

#include "../../glbackend/glbackend.h"

// def/clipmap handling


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

