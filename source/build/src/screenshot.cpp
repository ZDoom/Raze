
#include "compat.h"
#include "build.h"
#include "editor.h"

#ifdef USE_LIBPNG
//# include <setjmp.h>
# include <png.h>
#endif

int16_t capturecount = 0;

//
// screencapture
//

static int32_t screencapture_common1(char *fn, const char *ext, BFILE** filptr)
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

#ifdef USE_LIBPNG
// PNG screenshots -- adapted from libpng example.c
static int32_t screencapture_png(const char *filename, char inverseit, const char *versionstr)
{
    BFILE *fp;
# ifdef USE_OPENGL
#  define HICOLOR (getrendermode() >= REND_POLYMOST && in3dmode())
# else
#  define HICOLOR 0
# endif
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp volatile palette = NULL;
    png_textp volatile text = NULL;

    png_bytep volatile buf = NULL;
    png_bytepp volatile rowptrs = NULL;

    bssize_t const s_xdim = xdim, s_ydim = ydim;

    char fn[32];  // careful...

    Bstrcpy(fn, filename);
    int32_t const retval = screencapture_common1(fn, "png", &fp);
    if (retval)
        return retval;

    /* Create and initialize the png_struct with default error handling. */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        Bfclose(fp);
        return -1;
    }

    /* Allocate/initialize the image information data. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        Bfclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);
        return -1;
    }

    /* Set error handling. */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        /* If we get here, we had a problem writing the file */
        Bfclose(fp);
        if (palette) png_free(png_ptr, palette);
        if (text) png_free(png_ptr, text);
        if (buf) png_free(png_ptr, buf);
        if (rowptrs) png_free(png_ptr, rowptrs);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        return -1;
    }

    png_init_io(png_ptr, fp);

    // initialize various info fields from here on
    png_set_IHDR(png_ptr, info_ptr, xdim, ydim, 8,
        HICOLOR ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (HICOLOR && editstatus==0)
        png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_VALUE_NONE);

    if (!HICOLOR)
#if (PNG_LIBPNG_VER > 10599)
        palette = (png_colorp) png_malloc(png_ptr, 256*sizeof(png_color));
#else
        palette = (png_colorp) png_malloc(png_ptr, 256*png_sizeof(png_color));
#endif

    if (palette)
    {
        if (inverseit)
        {
            for (bssize_t i = 0; i < 256; ++i)
            {
                palette[i].red = 255 - curpalettefaded[i].r;
                palette[i].green = 255 - curpalettefaded[i].g;
                palette[i].blue = 255 - curpalettefaded[i].b;
            }
        }
        else
        {
            for (bssize_t i = 0; i < 256; ++i)
            {
                palette[i].red = curpalettefaded[i].r;
                palette[i].green = curpalettefaded[i].g;
                palette[i].blue = curpalettefaded[i].b;
            }
        }

        png_set_PLTE(png_ptr, info_ptr, palette, 256);
    }

    //    png_set_gAMA(png_ptr, info_ptr, vid_gamma);  // 1.0/vid_gamma ?
    //    png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_SATURATION);  // hm...

#if (PNG_LIBPNG_VER > 10599)
    text = (png_textp) png_malloc(png_ptr, 2*sizeof(png_text));
#else
    text = (png_textp) png_malloc(png_ptr, 2*png_sizeof(png_text));
#endif
    text[0].compression = PNG_TEXT_COMPRESSION_NONE;
    text[0].key = Bstrdup("Title");
    text[0].text = Bstrdup((editstatus ? "Mapster32 screenshot" : "EDuke32 screenshot"));

    text[1].compression = PNG_TEXT_COMPRESSION_NONE;
    text[1].key = Bstrdup("Software");
    text[1].text = Bstrdup(versionstr);
    png_set_text(png_ptr, info_ptr, text, 2);

    // get/set the pixel data
    begindrawing(); //{{{
    if (palette)
    {
        buf = (png_bytep) png_malloc(png_ptr, bytesperline*ydim);
        Bmemcpy(buf, (char *) frameplace, bytesperline*ydim);
    }
# ifdef USE_OPENGL
    else
    {
        buf = (png_bytep) png_malloc(png_ptr, xdim*ydim*3);
        bglReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buf);
    }
# endif
    enddrawing(); //}}}

    rowptrs = (png_bytepp) png_malloc(png_ptr, ydim*sizeof(png_bytep));

    if (!palette)
    {
        for (bssize_t i = 0; i < s_ydim; ++i)
            rowptrs[i] = &buf[3*s_xdim*(s_ydim-i-1)];
    }
    else
    {
        for (bssize_t i = 0; i < s_ydim; ++i)
            rowptrs[i] = &buf[ylookup[i]];
    }
    png_set_rows(png_ptr, info_ptr, rowptrs);

    // write the png file!
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    Bfclose(fp);
    if (palette) png_free(png_ptr, palette);

    DO_FREE_AND_NULL(text[0].key);
    DO_FREE_AND_NULL(text[0].text);

    DO_FREE_AND_NULL(text[1].key);
    DO_FREE_AND_NULL(text[1].text);

    if (text) png_free(png_ptr, text);

    if (buf) png_free(png_ptr, buf);
    if (rowptrs) png_free(png_ptr, rowptrs);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    OSD_Printf("Saved screenshot to %s\n", fn);
    capturecount++;
    return 0;
}
# undef HICOLOR

#else  // if !defined USE_LIBPNG

static int32_t screencapture_tga(const char *filename, char inverseit)
{
    int32_t i;
    char *ptr, head[18] = { 0,1,1,0,0,0,1,24,0,0,0,0,0/*wlo*/,0/*whi*/,0/*hlo*/,0/*hhi*/,8,0 };
    //char palette[4*256];
    char *fn = Xstrdup(filename);
# ifdef USE_OPENGL
    int32_t j;
    char *inversebuf;
# endif
    BFILE *fil;

    i = screencapture_common1(fn, "tga", &fil);
    if (i)
    {
        Bfree(fn);
        return i;
    }

# ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode())
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
# endif

    head[12] = xdim & 0xff;
    head[13] = (xdim >> 8) & 0xff;
    head[14] = ydim & 0xff;
    head[15] = (ydim >> 8) & 0xff;

    Bfwrite(head, 18, 1, fil);

    begindrawing(); //{{{
    ptr = (char *) frameplace;

    // palette first
# ifdef USE_OPENGL
    if (getrendermode() < REND_POLYMOST || (getrendermode() >= REND_POLYMOST && !in3dmode()))
# endif
    {
        //getpalette(0,256,palette);
        for (i=0; i<256; i++)
        {
            Bfputc(inverseit ? 255-curpalettefaded[i].b : curpalettefaded[i].b, fil);  // b
            Bfputc(inverseit ? 255-curpalettefaded[i].g : curpalettefaded[i].g, fil);  // g
            Bfputc(inverseit ? 255-curpalettefaded[i].r : curpalettefaded[i].r, fil);  // r
        }
    }

# ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        char c;
        // 24bit
        inversebuf = (char *) Xmalloc(xdim*ydim*3);

        bglReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, inversebuf);
        j = xdim*ydim*3;
        for (i=0; i<j; i+=3)
        {
            c = inversebuf[i];
            inversebuf[i] = inversebuf[i+2];
            inversebuf[i+2] = c;
        }
        Bfwrite(inversebuf, xdim*ydim, 3, fil);

        Bfree(inversebuf);
    }
    else
# endif
    {
        for (i=ydim-1; i>=0; i--)
            Bfwrite(ptr+i*bytesperline, xdim, 1, fil);
    }

    enddrawing();   //}}}

    Bfclose(fil);
    OSD_Printf("Saved screenshot to %s\n", fn);
    Bfree(fn);
    capturecount++;
    return 0;
}
# endif


int32_t screencapture(const char *filename, char inverseit, const char *versionstr)
{
#ifndef USE_LIBPNG
    UNREFERENCED_PARAMETER(versionstr);
    return screencapture_tga(filename, inverseit);
#else
    return screencapture_png(filename, inverseit, versionstr);
#endif
}
