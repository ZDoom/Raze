#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "version.h"
#include "m_png.h"
#include "i_specialpaths.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "gamecontrol.h"
#include "printf.h"

#include "../../glbackend/glbackend.h"

EXTERN_CVAR(Float, png_gamma)
//
// screencapture
//

FileWriter *OutputFileCounter::opennextfile(char *fn, char *zeros)
{
    do      // JBF 2004022: So we don't overwrite existing screenshots
    {
        if (count > 9999) return nullptr;

        zeros[0] = ((count/1000)%10)+'0';
        zeros[1] = ((count/100)%10)+'0';
        zeros[2] = ((count/10)%10)+'0';
        zeros[3] = (count%10)+'0';
		if (!FileExists(fn)) break;
        count++;
    } while (1);

    return FileWriter::Open(fn);
}

FileWriter *OutputFileCounter::opennextfile_withext(char *fn, const char *ext)
{
    char *dot = strrchr(fn, '.');
    strcpy(dot+1, ext);
	return opennextfile(fn, dot-4);
}

static OutputFileCounter capturecounter;

# ifdef USE_OPENGL
#  define HICOLOR (videoGetRenderMode() >= REND_POLYMOST && in3dmode())
# else
#  define HICOLOR 0
# endif

void getScreen(uint8_t* imgBuf)
{
	GLInterface.ReadPixels(xdim, ydim, imgBuf);
}


CVAR(String, screenshotname, "", CVAR_ARCHIVE)	// not GLOBALCONFIG - allow setting this per game.
CVAR(String, screenshot_dir, "", CVAR_ARCHIVE)					// same here.

//
// WritePNGfile
//
void WritePNGfile(FileWriter* file, const uint8_t* buffer, const PalEntry* palette,
	ESSType color_type, int width, int height, int pitch, float gamma)
{
	FStringf software("Demolition %s", GetVersionString());
	if (!M_CreatePNG(file, buffer, palette, color_type, width, height, pitch, gamma) ||
		!M_AppendPNGText(file, "Software", software) ||
		!M_FinishPNG(file))
	{
		Printf("Failed writing screenshot\n");
	}
}


int videoCaptureScreen()
{
	PalEntry Palette[256];

	size_t dirlen;
	FString autoname = Args->CheckValue("-shotdir");
	if (autoname.IsEmpty())
	{
		autoname = screenshot_dir;
	}
	dirlen = autoname.Len();
	if (dirlen == 0)
	{
		autoname = M_GetScreenshotsPath();
		dirlen = autoname.Len();
	}
	if (dirlen > 0)
	{
		if (autoname[dirlen - 1] != '/' && autoname[dirlen - 1] != '\\')
		{
			autoname += '/';
		}
	}
	autoname = NicePath(autoname);
	CreatePath(autoname);

	if (**screenshotname) autoname << screenshotname;
	else autoname << currentGame;
	autoname << "_0000";
	char* fn = autoname.LockBuffer();
    FileWriter *fil = capturecounter.opennextfile_withext(fn, "png");
	autoname.UnlockBuffer();

	if (fil == nullptr)
    {
        return -1;
    }

    uint8_t * const imgBuf = (uint8_t *) Xmalloc(xdim * ydim * (HICOLOR ? 3 : 1));

    videoBeginDrawing(); //{{{

    if (HICOLOR)
    {
        getScreen(imgBuf);
        int const bytesPerLine = xdim * 3;

        // flip rows
        uint8_t* rowBuf = (uint8_t *) Xmalloc(bytesPerLine);

        for (int i = 0, numRows = ydim >> 1; i < numRows; ++i)
        {
            memcpy(rowBuf, imgBuf + i * bytesPerLine, bytesPerLine);
            memcpy(imgBuf + i * bytesPerLine, imgBuf + (ydim - i - 1) * bytesPerLine, bytesPerLine);
            memcpy(imgBuf + (ydim - i - 1) * bytesPerLine, rowBuf, bytesPerLine);
        }

        Xfree(rowBuf);
    }
    else
    {
		for (bssize_t i = 0; i < 256; ++i)
		{
			Palette[i].r = curpalettefaded[i].r;
			Palette[i].g = curpalettefaded[i].g;
			Palette[i].b = curpalettefaded[i].b;
		}

        for (int i = 0; i < ydim; ++i)
            Bmemcpy(imgBuf + i * xdim, (uint8_t *)frameplace + ylookup[i], xdim);
    }

    videoEndDrawing(); //}}}

	WritePNGfile(fil, imgBuf, Palette, HICOLOR ? SS_RGB : SS_PAL, xdim, ydim, HICOLOR? xdim*3 : xdim, png_gamma);
	delete fil;
    Xfree(imgBuf);
	Printf("Saved screenshot to %s\n", fn);
	capturecounter.count++;

    return 0;
}

#undef HICOLOR

