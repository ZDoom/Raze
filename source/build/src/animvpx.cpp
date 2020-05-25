/* ANM file replacement with VP8 video */

#ifdef USE_LIBVPX

#include "compat.h"

#include "compat.h"
#include "baselayer.h"
#include "build.h"

#include "matrix.h"
#include "../../glbackend/glbackend.h"
#include "textures.h"
#include "bitmap.h"
#include "v_draw.h"
#include "v_video.h"

#undef UNUSED
#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "animvpx.h"


class VPXTexture : public FTexture
{
public:
    VPXTexture();
    void SetFrame(const void* data, int width, int height);
    virtual FBitmap GetBgraBitmap(const PalEntry* remap, int* trans) override;

protected:

    const void* data;

public:

};

//==========================================================================
//
//
//
//==========================================================================

VPXTexture::VPXTexture() {}

void VPXTexture::SetFrame(const void *data_, int width, int height)
{
    Width = width;
    Height = height;
    data = data_;
    SystemTextures.Clean(true, true);
}

//===========================================================================
//
// FPNGTexture::CopyPixels
//
//===========================================================================

FBitmap VPXTexture::GetBgraBitmap(const PalEntry* remap, int* trans)
{
    FBitmap bmp;

    bmp.Create(Width, Height);

    auto spix = (uint8_t*)data;
    auto dpix = bmp.GetPixels();
    for (int i = 0; i < Width * Height; i++)
    {
        int p = i * 4;
        float y = spix[p] * (1/255.f);
        float u = spix[p+1] * (1 / 255.f) - 0.5f;
        float v = spix[p+2] * (1 / 255.f) - 0.5f;

        y = 1.1643f * (y - 0.0625f);

        float r = y + 1.5958f * v;
        float g = y - 0.39173f * u - 0.81290f * v;
        float b = y + 2.017f * u;

        dpix[p + 0] =  (uint8_t)(clamp(b, 0, 1.f) * 255);
        dpix[p + 1] =  (uint8_t)(clamp(g, 0, 1.f) * 255);
        dpix[p + 2] =  (uint8_t)(clamp(r, 0, 1.f) * 255);
        dpix[p + 3] = 255;
    }
    return bmp;
}


const char *animvpx_read_ivf_header_errmsg[] = {
    "All OK",
    "couldn't read 32-byte IVF header",
    "magic mismatch, not an IVF file",
    "unrecognized IVF version, expected 0",
    "only VP8 video stream supported",
    "invalid framerate numerator or denominator after correction, must not be 0",
};

EDUKE32_STATIC_ASSERT(sizeof(animvpx_ivf_header_t) == 32);

int32_t animvpx_read_ivf_header(FileReader & inhandle, animvpx_ivf_header_t *hdr)
{
    int32_t err;

    if (inhandle.Read(hdr, sizeof(animvpx_ivf_header_t)) != sizeof(animvpx_ivf_header_t))
        return 1;  // "couldn't read header"

    err = animvpx_check_header(hdr);
    if (err)
        return err;

    hdr->hdrlen = B_LITTLE16(hdr->hdrlen);

    hdr->width = B_LITTLE16(hdr->width);
    hdr->height = B_LITTLE16(hdr->height);
    hdr->fpsnumer = B_LITTLE16(hdr->fpsnumer);
    hdr->fpsdenom = B_LITTLE16(hdr->fpsdenom);

    hdr->numframes = B_LITTLE32(hdr->numframes);

    // the rest is based on vpxdec.c --> file_is_ivf()

    if (hdr->fpsnumer < 1000)
    {
        // NOTE: We got rid of the 1/(2*fps) correction from libvpx's vpxdec.c,
        // users are encouraged to use the "ivfrate" utility instead

        if (hdr->fpsdenom==0 || hdr->fpsnumer==0)
            return 5;  // "invalid framerate numerator or denominator"

        Printf("animvpx: rate is %d frames / %d seconds (%.03f fps).\n",
                   hdr->fpsnumer, hdr->fpsdenom, (double)hdr->fpsnumer/hdr->fpsdenom);
    }
    else
    {
        double fps = (hdr->fpsdenom==0) ? 0.0 : (double)hdr->fpsnumer/hdr->fpsdenom;

        Printf("animvpx: set rate to 30 fps (header says %d frames / %d seconds = %.03f fps).\n",
                   hdr->fpsnumer, hdr->fpsdenom, fps);

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
int32_t animvpx_init_codec(const animvpx_ivf_header_t *info, FileReader & inhandle, animvpx_codec_ctx *codec)
{
    vpx_codec_dec_cfg_t cfg;

    cfg.threads = 1;
    cfg.w = info->width;
    cfg.h = info->height;

    codec->width = info->width;
    codec->height = info->height;

    //
    codec->inhandle = &inhandle;
    codec->pic = (uint8_t *)Xcalloc(info->width*info->height,4);

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

    codec->numframes = 0;
    Bmemset(codec->sumtimes, 0, sizeof(codec->sumtimes));
    Bmemset(codec->maxtimes, 0, sizeof(codec->maxtimes));

    return 0;
}

int32_t animvpx_uninit_codec(animvpx_codec_ctx *codec)
{
    if (codec->initstate <= 0)
        return 2;

    DO_FREE_AND_NULL(codec->pic);

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
static int32_t animvpx_read_frame(FileReader & inhandle, uint8_t **bufptr, uint32_t *bufsizptr, uint32_t *bufallocsizptr)
{
#pragma pack(push,1)
    struct { uint32_t framesiz; uint64_t timestamp; } hdr;
#pragma pack(pop)

    if (inhandle.Read(&hdr, sizeof(hdr)) != sizeof(hdr))
        return 1;

    if (hdr.framesiz == 0)
        return 6;  // must be 6, see animvpx_nextpic_errmsg[]

//    Printf("frame size: %u\n", hdr.framesiz);

    if (!*bufptr)
    {
        *bufptr = (uint8_t *)Xmalloc(hdr.framesiz);
        if (!*bufptr)
            return 2;
        *bufallocsizptr = hdr.framesiz;
    }
    else if (*bufallocsizptr < hdr.framesiz)
    {
        *bufptr = (uint8_t *)Xrealloc(*bufptr, hdr.framesiz);
        if (!*bufptr)
            return 2;
        *bufallocsizptr = hdr.framesiz;
    }

    *bufsizptr = hdr.framesiz;

    if (inhandle.Read(*bufptr, hdr.framesiz) != (signed)hdr.framesiz)
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
#ifdef DEBUGGINGAIDS
ATTRIBUTE_OPTIMIZE("O1")
#else
ATTRIBUTE_OPTIMIZE("O3")
#endif
int32_t animvpx_nextpic(animvpx_codec_ctx *codec, uint8_t **picptr)
{
    int32_t ret, corrupted;
    vpx_image_t *img;

    int32_t t[3];

    if (codec->initstate <= 0)  // not inited or error
        return 1;

    t[0] = timerGetTicks();

    if (codec->decstate == 0)  // first time / begin
    {
read_ivf_frame:
        corrupted = 0;

        ret = animvpx_read_frame(*codec->inhandle, &codec->compbuf, &codec->compbuflen,
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

// Compilation fix for Debian 6.0 (squeeze), which doesn't have
// VP8D_GET_FRAME_CORRUPTED.
// LibVPX doesn't seem to have a version #define, so we use the
// following one to determine conditional compilation.
#ifdef VPX_CODEC_CAP_ERROR_CONCEALMENT
        if (vpx_codec_control(&codec->codec, VP8D_GET_FRAME_CORRUPTED, &corrupted))
        {
            get_codec_error(codec);
            codec->decstate = -2;
            return 7;
        }
#endif
        if (corrupted)
            Printf("warning: corrupted frame!\n");
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

    t[1] = timerGetTicks();

    uint8_t *const dstpic = codec->pic;

    uint8_t const *const yplane = img->planes[VPX_PLANE_Y];
    uint8_t const *const uplane = img->planes[VPX_PLANE_U];
    uint8_t const *const vplane = img->planes[VPX_PLANE_V];

    const int32_t ystride = img->stride[VPX_PLANE_Y];
    const int32_t ustride = img->stride[VPX_PLANE_U];
    const int32_t vstride = img->stride[VPX_PLANE_V];

     /*** 3 planes --> packed conversion ***/
    vec2u_t const dim = { img->d_w, img->d_h };

    for (unsigned int y = 0; y < dim.y; y += 2)
    {
        unsigned int const y1 = y + 1;
        unsigned int const wy = dim.x * y;
        unsigned int const wy1 = dim.x * y1;

        for (unsigned int x = 0; x < dim.x; x += 2)
        {
            uint8_t u = uplane[ustride * (y >> 1) + (x >> 1)];
            uint8_t v = vplane[vstride * (y >> 1) + (x >> 1)];

            dstpic[(wy + x) << 2] = yplane[ystride * y + x];
            dstpic[(wy + x + 1) << 2] = yplane[ystride * y + x + 1];
            dstpic[(wy1 + x) << 2] = yplane[ystride * y1 + x];
            dstpic[(wy1 + x + 1) << 2] = yplane[ystride * y1 + x + 1];

            dstpic[((wy + x) << 2) + 1] = u;
            dstpic[((wy + x + 1) << 2) + 1] = u;
            dstpic[((wy1 + x) << 2) + 1] = u;
            dstpic[((wy1 + x + 1) << 2) + 1] = u;

            dstpic[((wy + x) << 2) + 2] = v;
            dstpic[((wy + x + 1) << 2) + 2] = v;
            dstpic[((wy1 + x) << 2) + 2] = v;
            dstpic[((wy1 + x + 1) << 2) + 2] = v;
        }
    }
    t[2] = timerGetTicks();

    codec->sumtimes[0] += t[1]-t[0];
    codec->sumtimes[1] += t[2]-t[1];

    codec->maxtimes[0] = max(codec->maxtimes[0], t[1]-t[0]);
    codec->maxtimes[1] = max(codec->maxtimes[0], t[2]-t[1]);

    *picptr = codec->pic;
    return 0;
}


/////////////// DRAWING! ///////////////
static int sampler;
static VPXTexture* vpxtex[2];
static int which;

void animvpx_setup_glstate(int32_t animvpx_flags)
{
    ////////// GL STATE //////////
    vpxtex[0] = new VPXTexture;
    vpxtex[1] = new VPXTexture;

    if ((animvpx_flags & CUTSCENE_TEXTUREFILTER && hw_texfilter == TEXFILTER_ON) || animvpx_flags & CUTSCENE_FORCEFILTER ||
    (!(animvpx_flags & CUTSCENE_TEXTUREFILTER) && !(animvpx_flags & CUTSCENE_FORCENOFILTER))) // if no flags, then use filter for IVFs
    {
		sampler = SamplerClampXY;
    }
    else
    {
		sampler = SamplerNoFilterClampXY;
    }


	GLInterface.ClearScreen(0, true);
}

void animvpx_restore_glstate(void)
{
	delete vpxtex[0];
	vpxtex[0] = nullptr;
    delete vpxtex[1];
    vpxtex[1] = nullptr;
}

int32_t animvpx_render_frame(animvpx_codec_ctx *codec, double animvpx_aspect)
{
    int32_t t = timerGetTicks();

    if (codec->initstate <= 0)  // not inited or error
        return 1;

    if (codec->pic == NULL)
        return 2;  // shouldn't happen

    which ^= 1;
    vpxtex[which]->SetFrame(codec->pic, codec->width, codec->height);

    float vid_wbyh = ((float)codec->width)/codec->height;
    if (animvpx_aspect > 0)
        vid_wbyh = animvpx_aspect;
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

    x *= screen->GetWidth() / 2;
    y *= screen->GetHeight() / 2;
    DrawTexture(twod, vpxtex[which], screen->GetWidth() / 2 - int(x), screen->GetHeight()/2 - int(y), DTA_DestWidth, 2*int(x), DTA_DestHeight, 2*int(y), 
        DTA_Masked, false, DTA_KeepRatio, true, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

    t = timerGetTicks()-t;
    codec->sumtimes[2] += t;
    codec->maxtimes[2] = max(codec->maxtimes[2], t);
    codec->numframes++;

    return 0;
}

void animvpx_print_stats(const animvpx_codec_ctx *codec)
{
    if (codec->numframes != 0)
    {
        const int32_t *s = codec->sumtimes;
        const int32_t *m = codec->maxtimes;
        int32_t n = codec->numframes;

        Printf("VP8 timing stats (mean, max) [ms] for %d frames:\n"
                   " read and decode frame: %.02f, %d\n"
                   " 3 planes -> packed conversion: %.02f, %d\n"
                   " upload and display: %.02f, %d\n",
                   n, (double)s[0]/n, m[0], (double)s[1]/n, m[1], (double)s[2]/n, m[2]);
    }
}

#endif
