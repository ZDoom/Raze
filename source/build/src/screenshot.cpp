#include "compat.h"
#include "build.h"
#include "editor.h"

#include "pngwrite.h"

#include "vfs.h"

//
// screencapture
//

buildvfs_FILE OutputFileCounter::opennextfile(char *fn, char *zeros)
{
    buildvfs_FILE file;

    do      // JBF 2004022: So we don't overwrite existing screenshots
    {
        if (count > 9999) return nullptr;

        zeros[0] = ((count/1000)%10)+'0';
        zeros[1] = ((count/100)%10)+'0';
        zeros[2] = ((count/10)%10)+'0';
        zeros[3] = (count%10)+'0';

        if ((file = buildvfs_fopen_read(fn)) == nullptr) break;
        buildvfs_fclose(file);
        count++;
    } while (1);

    return buildvfs_fopen_write(fn);
}

buildvfs_FILE OutputFileCounter::opennextfile_withext(char *fn, const char *ext)
{
    char *dot = strrchr(fn, '.');
    strcpy(dot+1, ext);
    return opennextfile(fn, dot-4);
}

static OutputFileCounter capturecounter;

static void screencapture_end(char *fn, buildvfs_FILE * filptr)
{
    buildvfs_fclose(*filptr);
    OSD_Printf("Saved screenshot to %s\n", fn);
    Xfree(fn);
    capturecounter.count++;
}

# ifdef USE_OPENGL
#  define HICOLOR (videoGetRenderMode() >= REND_POLYMOST && in3dmode())
# else
#  define HICOLOR 0
# endif

int videoCaptureScreen(const char *filename, char inverseit)
{
    char *fn = Xstrdup(filename);
    buildvfs_FILE fp = capturecounter.opennextfile_withext(fn, "png");

    if (fp == nullptr)
    {
        Xfree(fn);
        return -1;
    }

    uint8_t * const imgBuf = (uint8_t *) Xmalloc(xdim * ydim * (HICOLOR ? 3 : 1));

    videoBeginDrawing(); //{{{

#ifdef USE_OPENGL
    if (HICOLOR)
    {
        glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, imgBuf);
        int const bytesPerLine = xdim * 3;

        if (inverseit)
        {
            for (int i=0, j = ydim * bytesPerLine; i<j; i+=3)
                swapchar(&imgBuf[i], &imgBuf[i+2]);
        }

        // flip rows
        uint8_t* rowBuf = (uint8_t *) Xmalloc(bytesPerLine);

        for (int i = 0, numRows = ydim >> 1; i < numRows; ++i)
        {
            Bmemcpy(rowBuf, imgBuf + i * bytesPerLine, bytesPerLine);
            Bmemcpy(imgBuf + i * bytesPerLine, imgBuf + (ydim - i - 1) * bytesPerLine, bytesPerLine);
            Bmemcpy(imgBuf + (ydim - i - 1) * bytesPerLine, rowBuf, bytesPerLine);
        }

        Xfree(rowBuf);
    }
    else
#endif
    {
        struct {
            uint8_t r, g, b;
        } palette[256];

        if (inverseit)
        {
            for (bssize_t i = 0; i < 256; ++i)
            {
                palette[i].r = 255 - curpalettefaded[i].r;
                palette[i].g = 255 - curpalettefaded[i].g;
                palette[i].b = 255 - curpalettefaded[i].b;
            }
        }
        else
        {
            for (bssize_t i = 0; i < 256; ++i)
                Bmemcpy(&palette[i], &curpalettefaded[i], sizeof(palette[0]));
        }

        png_set_pal((uint8_t *)palette, 256);

        for (int i = 0; i < ydim; ++i)
            Bmemcpy(imgBuf + i * xdim, (uint8_t *)frameplace + ylookup[i], xdim);
    }

    videoEndDrawing(); //}}}

    png_set_text("Software", osd->version.buf);
    png_write(fp, xdim, ydim, HICOLOR ? PNG_TRUECOLOR : PNG_INDEXED, imgBuf);
    Xfree(imgBuf);
    screencapture_end(fn, &fp);

    return 0;
}

int videoCaptureScreenTGA(const char *filename, char inverseit)
{
    int32_t i;
    char head[18] = { 0,1,1,0,0,0,1,24,0,0,0,0,0/*wlo*/,0/*whi*/,0/*hlo*/,0/*hhi*/,8,0 };
    //char palette[4*256];
    char *fn = Xstrdup(filename);

    buildvfs_FILE fil = capturecounter.opennextfile_withext(fn, "tga");
    if (fil == nullptr)
    {
        Xfree(fn);
        return -1;
    }

#ifdef USE_OPENGL
    if (HICOLOR)
    {
        head[1] = 0;    // no colourmap
        head[2] = 2;    // uncompressed truecolour
        head[3] = 0;    // (low) first colourmap index
        head[4] = 0;    // (high) first colourmap index
        head[5] = 0;    // (low) number colourmap entries
        head[6] = 0;    // (high) number colourmap entries
        head[7] = 0;    // colourmap entry size
        head[16] = 24;  // 24 bits per pixel
    }
#endif

    head[12] = xdim & 0xff;
    head[13] = (xdim >> 8) & 0xff;
    head[14] = ydim & 0xff;
    head[15] = (ydim >> 8) & 0xff;

    buildvfs_fwrite(head, 18, 1, fil);

    // palette first
#ifdef USE_OPENGL
    if (!HICOLOR)
#endif
    {
        if (inverseit)
        {
            for (i=0; i<256; i++)
            {
                buildvfs_fputc(255 - curpalettefaded[i].b, fil);
                buildvfs_fputc(255 - curpalettefaded[i].g, fil);
                buildvfs_fputc(255 - curpalettefaded[i].r, fil);
            }
        }
        else
        {
            for (i=0; i<256; i++)
            {
                buildvfs_fputc(curpalettefaded[i].b, fil);
                buildvfs_fputc(curpalettefaded[i].g, fil);
                buildvfs_fputc(curpalettefaded[i].r, fil);
            }
        }
    }

    videoBeginDrawing(); //{{{

# ifdef USE_OPENGL
    if (HICOLOR)
    {
        // 24bit
        int const size = xdim * ydim * 3;
        uint8_t *inversebuf = (uint8_t *) Xmalloc(size);

        glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, inversebuf);

        for (i = 0; i < size; i += 3)
            swapchar(&inversebuf[i], &inversebuf[i + 2]);

        buildvfs_fwrite(inversebuf, xdim*ydim, 3, fil);
        Xfree(inversebuf);
    }
    else
# endif
    {
        char * const ptr = (char *) frameplace;

        for (i = ydim-1; i >= 0; i--)
            buildvfs_fwrite(ptr + i * bytesperline, xdim, 1, fil);
    }

    videoEndDrawing();   //}}}

    screencapture_end(fn, &fil);

    return 0;
}
#undef HICOLOR

