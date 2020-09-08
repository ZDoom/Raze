/* ANM file replacement with VP8 video */

#ifdef USE_LIBVPX

#include "compat.h"

#include "compat.h"
#include "build.h"
#include "printf.h"
#include "matrix.h"
#include "../../glbackend/glbackend.h"
#include "textures.h"
#include "bitmap.h"
#include "v_draw.h"
#include "v_video.h"
#include "texturemanager.h"

#undef UNUSED
#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "animvpx.h"

struct vec2u_t
{
    uint32_t x, y;
} ;

const char *animvpx_read_ivf_header_errmsg[] = {
    "All OK",
    "couldn't read 32-byte IVF header",
    "magic mismatch, not an IVF file",
    "unrecognized IVF version, expected 0",
    "only VP8 video stream supported",
    "invalid framerate numerator or denominator after correction, must not be 0",
};

static_assert(sizeof(animvpx_ivf_header_t) == 32);

inline int32_t animvpx_check_header(const animvpx_ivf_header_t* hdr)
{
    if (memcmp(hdr->magic, "DKIF", 4))
        return 2;  // "not an IVF file"

    if (hdr->version != 0)
        return 3;  // "unrecognized IVF version"

    // fourcc is left as-is
    if (memcmp(hdr->fourcc, "VP80", 4))
        return 4;  // "only VP8 supported"

    return 0;
}


int32_t animvpx_read_ivf_header(FileReader & inhandle, animvpx_ivf_header_t *hdr)
{
    int32_t err;

    if (inhandle.Read(hdr, sizeof(animvpx_ivf_header_t)) != sizeof(animvpx_ivf_header_t))
        return 1;  // "couldn't read header"

    err = animvpx_check_header(hdr);
    if (err)
        return err;

    hdr->hdrlen = LittleShort(hdr->hdrlen);

    hdr->width = LittleShort(hdr->width);
    hdr->height = LittleShort(hdr->height);
    hdr->fpsnumer = LittleLong(hdr->fpsnumer);
    hdr->fpsdenom = LittleLong(hdr->fpsdenom);

    hdr->numframes = LittleLong(hdr->numframes);

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
    memset(codec->sumtimes, 0, sizeof(codec->sumtimes));
    memset(codec->maxtimes, 0, sizeof(codec->maxtimes));

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

int32_t animvpx_nextpic(animvpx_codec_ctx *codec, uint8_t **picptr)
{
    int32_t ret, corrupted;
    vpx_image_t *img;

    int32_t t[3];

    if (codec->initstate <= 0)  // not inited or error
        return 1;

    t[0] = I_msTime();

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

    t[1] = I_msTime();

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
    t[2] = I_msTime();

    codec->sumtimes[0] += t[1]-t[0];
    codec->sumtimes[1] += t[2]-t[1];

    codec->maxtimes[0] = max(codec->maxtimes[0], t[1]-t[0]);
    codec->maxtimes[1] = max(codec->maxtimes[0], t[2]-t[1]);

    *picptr = codec->pic;
    return 0;
}

#endif
