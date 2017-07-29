#include "compat.h"
#include "build.h"
#include "editor.h"

#include "pngwrite.h"

uint16_t capturecount = 0;

//
// screencapture
//

static int screencapture_begin(char *fn, const char *ext, BFILE** filptr)
{
    bssize_t i;

    do      // JBF 2004022: So we don't overwrite existing screenshots
    {
        if (capturecount > 9999) return -1;

        i = Bstrrchr(fn, '.')-fn-4;
        fn[i++] = ((capturecount/1000)%10)+48;
        fn[i++] = ((capturecount/100)%10)+48;
        fn[i++] = ((capturecount/10)%10)+48;
        fn[i++] = (capturecount%10)+48;
        i++;
        Bstrcpy(&fn[i], ext);

        if ((*filptr = Bfopen(fn, "rb")) == NULL) break;
        Bfclose(*filptr);
        capturecount++;
    } while (1);

    *filptr = Bfopen(fn, "wb");
    if (*filptr == NULL)
        return -1;

    return 0;
}

static void screencapture_end(char *fn, BFILE** filptr)
{
    Bfclose(*filptr);
    OSD_Printf("Saved screenshot to %s\n", fn);
    Bfree(fn);
    capturecount++;
}

# ifdef USE_OPENGL
#  define HICOLOR (getrendermode() >= REND_POLYMOST && in3dmode())
# else
#  define HICOLOR 0
# endif

int screencapture(const char *filename, char inverseit)
{
    char *fn = Xstrdup(filename);
    BFILE *fp;
    int const retval = screencapture_begin(fn, "png", &fp);

    if (retval)
    {
        Bfree(fn);
        return retval;
    }

    uint8_t * const imgBuf = (uint8_t *) Xmalloc(xdim * ydim * (HICOLOR ? 3 : 1));

    begindrawing(); //{{{

#ifdef USE_OPENGL
    if (HICOLOR)
    {
        bglReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, imgBuf);
        int const bytesPerLine = xdim * 3;

        if (inverseit)
        {
            for (bsize_t i=0, j = ydim * bytesPerLine; i<j; i+=3)
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

        Bfree(rowBuf);
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

    enddrawing(); //}}}

    png_set_text("Software", osd->version.buf);
    png_write(fp, xdim, ydim, HICOLOR ? PNG_TRUECOLOR : PNG_INDEXED, imgBuf);
    Bfree(imgBuf);
    screencapture_end(fn, &fp);

    return 0;
}

int screencapture_tga(const char *filename, char inverseit)
{
    int32_t i;
    char head[18] = { 0,1,1,0,0,0,1,24,0,0,0,0,0/*wlo*/,0/*whi*/,0/*hlo*/,0/*hhi*/,8,0 };
    //char palette[4*256];
    char *fn = Xstrdup(filename);
    BFILE *fil;

    i = screencapture_begin(fn, "tga", &fil);
    if (i)
    {
        Bfree(fn);
        return i;
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

    Bfwrite(head, 18, 1, fil);

    // palette first
#ifdef USE_OPENGL
    if (!HICOLOR)
#endif
    {
        if (inverseit)
        {
            for (i=0; i<256; i++)
            {
                Bfputc(255 - curpalettefaded[i].b, fil);
                Bfputc(255 - curpalettefaded[i].g, fil);
                Bfputc(255 - curpalettefaded[i].r, fil);
            }
        }
        else
        {
            for (i=0; i<256; i++)
            {
                Bfputc(curpalettefaded[i].b, fil);
                Bfputc(curpalettefaded[i].g, fil);
                Bfputc(curpalettefaded[i].r, fil);
            }
        }
    }

    begindrawing(); //{{{

# ifdef USE_OPENGL
    if (HICOLOR)
    {
        // 24bit
        int const size = xdim * ydim * 3;
        uint8_t *inversebuf = (uint8_t *) Xmalloc(size);

        bglReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, inversebuf);

        for (i = 0; i < size; i += 3)
            swapchar(&inversebuf[i], &inversebuf[i + 2]);

        Bfwrite(inversebuf, xdim*ydim, 3, fil);
        Bfree(inversebuf);
    }
    else
# endif
    {
        char * const ptr = (char *) frameplace;

        for (i = ydim-1; i >= 0; i--)
            Bfwrite(ptr + i * bytesperline, xdim, 1, fil);
    }

    enddrawing();   //}}}

    screencapture_end(fn, &fil);

    return 0;
}
#undef HICOLOR

