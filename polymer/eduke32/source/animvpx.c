/* ANM file replacement with VP8 video */

#include <stdint.h>

#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "glbuild.h"

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#include "duke3d.h"
#include "game.h"  // kopen4loadfrommod
#include "animvpx.h"

const char *animvpx_read_ivf_header_errmsg[] = {
    "All OK",
    "couldn't read 32-byte IVF header",
    "magic mismatch, not an IVF file",
    "unrecognized IVF version, expected 0",
    "only VP8 video stream supported",
    "invalid framerate numerator or denominator after correction, must not be 0",
    "INTERNAL ERROR, IVF header size wrong"
};

int32_t animvpx_read_ivf_header(int32_t inhandle, animvpx_ivf_header_t *hdr)
{
    if (sizeof(animvpx_ivf_header_t) != 32)
        return 6;

    if (kread(inhandle, hdr, sizeof(animvpx_ivf_header_t)) != sizeof(animvpx_ivf_header_t))
        return 1;  // "couldn't read header"

    if (Bmemcmp(hdr,"DKIF",4))
        return 2;  // "not an IVF file"

    hdr->version = B_LITTLE16(hdr->version);
    if (hdr->version != 0)
        return 3;  // "unrecognized IVF version"

    hdr->hdrlen = B_LITTLE16(hdr->hdrlen);
    // fourcc is left as-is

    if (Bmemcmp(hdr->fourcc, "VP80", 4))
        return 4;  // "only VP8 supported"

    hdr->width = B_LITTLE16(hdr->width);
    hdr->height = B_LITTLE16(hdr->height);
    hdr->fpsnumer = B_LITTLE16(hdr->fpsnumer);
    hdr->fpsdenom = B_LITTLE16(hdr->fpsdenom);

    hdr->numframes = B_LITTLE32(hdr->numframes);

    // the rest is snatched from vpxdec.c (except 0 check for fps)

    /* Some versions of vpxenc used 1/(2*fps) for the timebase, so
     * we can guess the framerate using only the timebase in this
     * case. Other files would require reading ahead to guess the
     * timebase, like we do for webm.
     */
    if (hdr->fpsnumer < 1000)
    {
        /* Correct for the factor of 2 applied to the timebase in the
         * encoder.
         */
        if (hdr->fpsnumer&1)
            hdr->fpsdenom <<= 1;
        else
            hdr->fpsnumer >>= 1;

        if (hdr->fpsdenom==0 || hdr->fpsnumer==0)
            return 5;  // "invalid framerate numerator or denominator"
    }
    else
    {
        /* Don't know FPS for sure, and don't have readahead code
         * (yet?), so just default to 30fps.
         */
        hdr->fpsnumer = 30;
        hdr->fpsdenom = 1;
    }

    return 0;
}

////////// CODEC STUFF //////////
static void get_codec_error(animvpx_codec_ctx *codec)
{
    codec->errmsg_detail = vpx_codec_error_detail(&codec->codec);
    codec->errmsg = vpx_codec_error(&codec->codec);
}

// no checks for double-init!
int32_t animvpx_init_codec(const animvpx_ivf_header_t *info, int32_t inhandle, animvpx_codec_ctx *codec)
{
    vpx_codec_dec_cfg_t cfg;

    cfg.threads = 1;
    cfg.w = info->width;
    cfg.h = info->height;

    codec->width = info->width;
    codec->height = info->height;

    //
    codec->inhandle = inhandle;
    codec->pic = Bcalloc(info->width*info->height,4);

    codec->compbuflen = codec->compbufallocsiz = 0;
    codec->compbuf = NULL;

    codec->iter = NULL;

    if (codec->pic == NULL)
    {
        codec->initstate = -1;
        return 1;
    }

    if (vpx_codec_dec_init(&codec->codec, &vpx_codec_vp8_dx_algo, &cfg, 0))
    {
        get_codec_error(codec);
        codec->initstate = -1;
        return 1;
    }

    codec->initstate = 1;
    codec->decstate = 0;

    codec->errmsg_detail = codec->errmsg = NULL;

    return 0;
}

int32_t animvpx_uninit_codec(animvpx_codec_ctx *codec)
{
    if (codec->initstate <= 0)
        return 2;

    Bfree(codec->pic);
    codec->pic = NULL;

    if (vpx_codec_destroy(&codec->codec))
    {
        get_codec_error(codec);
        codec->initstate = -2;
        return 1;
    }

    codec->initstate = 0;

    return 0;
}

////////// FRAME RETRIEVAL //////////

// read one IVF/VP8 frame, which may code multiple "picture-frames"
static int32_t animvpx_read_frame(int32_t inhandle, uint8_t **bufptr, uint32_t *bufsizptr, uint32_t *bufallocsizptr)
{
#pragma pack(push,1)
    struct { uint32_t framesiz; uint64_t timestamp; } hdr;
#pragma pack(pop)

    if (kread(inhandle, &hdr, sizeof(hdr)) != sizeof(hdr))
        return 1;

    if (hdr.framesiz == 0)
        return 6;  // must be 6, see animvpx_nextpic_errmsg[]

//    OSD_Printf("frame size: %u\n", hdr.framesiz);

    if (!*bufptr)
    {
        *bufptr = Bmalloc(hdr.framesiz);
        if (!*bufptr)
            return 2;
        *bufallocsizptr = hdr.framesiz;
    }
    else if (*bufallocsizptr < hdr.framesiz)
    {
        *bufptr = Brealloc(*bufptr, hdr.framesiz);
        if (!*bufptr)
            return 2;
        *bufallocsizptr = hdr.framesiz;
    }

    *bufsizptr = hdr.framesiz;

    if (kread(inhandle, *bufptr, hdr.framesiz) != (signed)hdr.framesiz)
        return 3;

    return 0;
}

const char *animvpx_nextpic_errmsg[] = {
    "All OK",
    "INTERNAL ERROR, animvpx_codec_ctx not initalized!",
    "OUT OF MEMORY",
    "couldn't read whole frame",
    "decoder error, couldn't decode frame",
    "picture dimension mismatch",
    "INTERNAL ERROR: read 0 frame length",
    "Failed getting corruption status (VP8D_GET_FRAME_CORRUPTED)"
};

// retrieves one picture-frame from the stream
//  pic format:  lines of [Y U V 0] pixels
//  *picptr==NULL means EOF has been reached
#ifndef USING_LTO
# ifdef DEBUGGINGAIDS
ATTRIBUTE((optimize("O1")))
# else
ATTRIBUTE((optimize("O3")))
# endif
#endif
int32_t animvpx_nextpic(animvpx_codec_ctx *codec, uint8_t **picptr)
{
    int32_t ret, corrupted;
    uint32_t x, y;
    vpx_image_t *img;

    if (codec->initstate <= 0)  // not inited or error
        return 1;

    if (codec->decstate == 0)  // first time / begin
    {
read_ivf_frame:
        corrupted = 0;

        ret = animvpx_read_frame(codec->inhandle, &codec->compbuf, &codec->compbuflen,
                                 &codec->compbufallocsiz);
        if (ret == 1)
        {
            // reached EOF
            *picptr = NULL;
            codec->decstate = 2;
            return 0;
        }
        else if (ret == 2 || ret == 3 || ret == 6)
        {
            *picptr = NULL;
            codec->decstate = -1;
            return ret;
        }
        // ^^^ keep in sync with all animvpx_read_frame() errors!

        // codec->compbuf now contains one IVF/VP8 frame
        codec->decstate = 1;

        // decode it!
        if (vpx_codec_decode(&codec->codec, codec->compbuf, codec->compbuflen, NULL, 0))
        {
            get_codec_error(codec);
            codec->decstate = -2;
            return 4;
        }

        if (vpx_codec_control(&codec->codec, VP8D_GET_FRAME_CORRUPTED, &corrupted))
        {
            get_codec_error(codec);
            codec->decstate = -2;
            return 7;
        }

        if (corrupted)
            OSD_Printf("warning: corrupted frame!\n");
    }

    img = vpx_codec_get_frame(&codec->codec, &codec->iter);
    if (img == NULL)
    {
        codec->iter = NULL;  // !
        goto read_ivf_frame;
    }

    if (img->d_w != codec->width || img->d_h != codec->height)
    {
        codec->decstate = -1;
        return 5;
    }

    /*** 3 planes --> packed conversion ***/
    for (y=0; y<img->d_h; y++)
        for (x=0; x<img->d_w; x++)
        {
            codec->pic[(img->d_w*y + x)<<2] = img->planes[VPX_PLANE_Y][img->stride[VPX_PLANE_Y]*y + x];
            codec->pic[((img->d_w*y + x)<<2) + 1] = img->planes[VPX_PLANE_U][img->stride[VPX_PLANE_U]*(y>>1) + (x>>1)];
            codec->pic[((img->d_w*y + x)<<2) + 2] = img->planes[VPX_PLANE_V][img->stride[VPX_PLANE_V]*(y>>1) + (x>>1)];
        }

    *picptr = codec->pic;
    return 0;
}


/////////////// DRAWING! ///////////////
static GLuint texname = 0;
static int32_t texuploaded;

// YUV->RGB conversion fragment shader adapted from
// http://www.fourcc.org/fccyvrgb.php: "Want some sample code?"
// direct link: http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
static char *fragprog_src =
    "#version 120\n"

    "uniform sampler2D tex;\n"

    "void main(void) {\n"

    "  float r,g,b,y,u,v;\n"
    "  vec3 yuv;\n"

    "  yuv = texture2D(tex, gl_TexCoord[0].st).rgb;\n"
    "  y = yuv.r;\n"
    "  u = yuv.g;\n"
    "  v = yuv.b;\n"

    "  y = 1.1643*(y-0.0625);\n"
    "  u = u-0.5;\n"
    "  v = v-0.5;\n"

    "  r = y + 1.5958*v;\n"
    "  g = y - 0.39173*u - 0.81290*v;\n"
    "  b = y + 2.017*u;\n"

    "  gl_FragColor = vec4(r,g,b,1.0);\n"
    "}\n";

void animvpx_setup_glstate(void)
{
    GLint gli;
    GLhandleARB FSHandle, PHandle;
    static char logbuf[512];

    // first, compile the fragment shader
    /* Set up program objects. */
    PHandle = bglCreateProgramObjectARB();
    FSHandle = bglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    /* Compile the shader. */
    bglShaderSourceARB(FSHandle, 1, (const GLcharARB **)&fragprog_src, NULL);
    bglCompileShaderARB(FSHandle);

    /* Print the compilation log. */
    bglGetObjectParameterivARB(FSHandle, GL_OBJECT_COMPILE_STATUS_ARB, &gli);
    bglGetInfoLogARB(FSHandle, sizeof(logbuf), NULL, logbuf);
    if (logbuf[0])
        OSD_Printf("animvpx compile log: %s\n", logbuf);

    /* Create a complete program object. */
    bglAttachObjectARB(PHandle, FSHandle);
    bglLinkProgramARB(PHandle);

    /* And print the link log. */
    bglGetInfoLogARB(PHandle, sizeof(logbuf), NULL, logbuf);
    if (logbuf[0])
        OSD_Printf("animvpx link log: %s\n", logbuf);

    /* Finally, use the program. */
    bglUseProgramObjectARB(PHandle);

    ////////// GL STATE //////////

    //Force fullscreen (glox1=-1 forces it to restore afterwards)
    bglViewport(0,0,xdim,ydim); glox1 = -1;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();

    bglMatrixMode(GL_COLOR);
    bglLoadIdentity();

    bglMatrixMode(GL_TEXTURE);
    bglLoadIdentity();

    bglPushAttrib(GL_ENABLE_BIT);
    bglDisable(GL_ALPHA_TEST);
//    bglDisable(GL_LIGHTING);
    bglDisable(GL_DEPTH_TEST);
    bglDisable(GL_BLEND);
    bglDisable(GL_CULL_FACE);
//    bglDisable(GL_SCISSOR_TEST);
    bglEnable(GL_TEXTURE_2D);


    bglActiveTextureARB(GL_TEXTURE0_ARB);
    bglGenTextures(1, &texname);
//    gli = bglGetUniformLocationARB(PHandle,"tex");
//    bglUniform1iARB(gli,0);  // 0: texture unit
    bglBindTexture(GL_TEXTURE_2D, texname);

    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    texuploaded = 0;
    ////////////////////

    bglClearColor(0.0,0.0,0.0,1.0);
    bglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void animvpx_restore_glstate(void)
{
    bglUseProgramObjectARB(0);

    bglPopAttrib();

    bglDeleteTextures(1, &texname);
    texname = 0;
    texuploaded = 0;
}

int32_t animvpx_render_frame(const animvpx_codec_ctx *codec)
{
    if (codec->initstate <= 0)  // not inited or error
        return 1;

    if (codec->pic == NULL)
        return 2;  // shouldn't happen

    if (!texuploaded)
    {
        bglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, codec->width,codec->height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, codec->pic);
        texuploaded = 1;
    }
    else
    {
        bglTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, codec->width,codec->height,
                        GL_RGBA, GL_UNSIGNED_BYTE, codec->pic);
    }

    {
        float vid_wbyh = ((float)codec->width)/codec->height;
        float scr_wbyh = ((float)xdim)/ydim;

        float x=1.0, y=1.0;
#if 1
        // aspect correction by pillarboxing/letterboxing
        // TODO: fullscreen? can't assume square pixels there
        if (vid_wbyh != scr_wbyh)
        {
            if (vid_wbyh < scr_wbyh)
                x = vid_wbyh/scr_wbyh;
            else
                y = scr_wbyh/vid_wbyh;
        }
#endif
        bglBegin(GL_QUADS);

        bglTexCoord2f(0.0,1.0);
        bglVertex3f(-x, -y, 0.0);

        bglTexCoord2f(0.0,0.0);
        bglVertex3f(-x, y, 0.0);

        bglTexCoord2f(1.0,0.0);
        bglVertex3f(x, y, 0.0);

        bglTexCoord2f(1.0,1.0);
        bglVertex3f(x, -y, 0.0);

        bglEnd();
    }

    return 0;
}
