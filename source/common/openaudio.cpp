

#include "openaudio.h"
#include "gamecvars.h"


//////////

#if 0 // disabled pending a rewrite from the ground up.

static FileReader S_TryFormats(char * const testfn, char * const fn_suffix, char const searchfirst)
{
#ifdef HAVE_FLAC
    {
        Bstrcpy(fn_suffix, ".flac");
        auto fp = fileSystem.OpenFileReader(testfn, searchfirst);
        if (fp.isOpen())
            return fp;
    }
#endif

#ifdef HAVE_VORBIS
    {
        Bstrcpy(fn_suffix, ".ogg");
		auto fp = fileSystem.OpenFileReader(testfn, searchfirst);
		if (fp.isOpen())
			return fp;
	}
#endif

    return FileReader();
}

static FileReader S_TryExtensionReplacements(char * const testfn, char const searchfirst, uint8_t const ismusic)
{
    char * extension = Bstrrchr(testfn, '.');
    char * const fn_end = Bstrchr(testfn, '\0');

    // ex: grabbag.voc --> grabbag_voc.*
    if (extension != NULL)
    {
        *extension = '_';

        auto fp = S_TryFormats(testfn, fn_end, searchfirst);
        if (fp.isOpen())
            return fp;
    }
    else
    {
        extension = fn_end;
    }

    // ex: grabbag.mid --> grabbag.*
    if (ismusic)
    {
        auto fp = S_TryFormats(testfn, extension, searchfirst);
		if (fp.isOpen())
			return fp;
    }

    return FileReader();
}
#endif

FileReader S_OpenAudio(const char *fn, char searchfirst, uint8_t const ismusic)
{
	auto origfp = fileSystem.OpenFileReader(fn, searchfirst);
	if (!snd_tryformats) return origfp;
	return origfp;
#if 0 // this needs to be redone
	char const* const origparent = origfp.isOpen() ? kfileparent(origfp) : NULL;
	uint32_t const    parentlength = origparent != NULL ? Bstrlen(origparent) : 0;

    auto testfn = (char *)Xmalloc(Bstrlen(fn) + 12 + parentlength); // "music/" + overestimation of parent minus extension + ".flac" + '\0'

    // look in ./
    // ex: ./grabbag.mid
    Bstrcpy(testfn, fn);
	auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
	if (fp.isOpen())
	{
		Bfree(testfn);
		return fp;
	}

    // look in ./music/<file's parent GRP name>/
    // ex: ./music/duke3d/grabbag.mid
    // ex: ./music/nwinter/grabbag.mid
    if (origparent != NULL)
    {
        char const * const parentextension = Bstrrchr(origparent, '.');
        uint32_t const namelength = parentextension != NULL ? (unsigned)(parentextension - origparent) : parentlength;

		Bsprintf(testfn, "music/%.*s/%s", namelength, origparent, fn);
		auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
		if (fp.isOpen())
		{
			Bfree(testfn);
			return fp;
		}
    }

	// look in ./music/
	// ex: ./music/grabbag.mid
	{
		Bsprintf(testfn, "music/%s", fn);
		auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
		if (fp.isOpen())
		{
			Bfree(testfn);
			return fp;
		}
	}

	Bfree(testfn);
	return origfp;
#endif
}

