#ifndef USE_OPENGL
# error "VP8 support requires OpenGL"
#endif

#ifndef ANIM_VPX_H
#define ANIM_VPX_H

#define VPX_CODEC_DISABLE_COMPAT 1
#ifndef ANIMVPX_STANDALONE
# undef UNUSED
# include <vpx/vpx_decoder.h>
//#include <vpx/vp8dx.h>
#endif

// IVF format: http://wiki.multimedia.cx/index.php?title=IVF
#pragma pack(push,1)
typedef struct
{
    char magic[4];
    uint16_t version;
    uint16_t hdrlen;

    char fourcc[4];
    uint16_t width;
    uint16_t height;

    uint32_t fpsnumer;
    uint32_t fpsdenom;

    uint32_t numframes;
    uint32_t unused_;
} animvpx_ivf_header_t;
#pragma pack(pop)

#ifndef ANIMVPX_STANDALONE
#include "vfs.h"

extern const char *animvpx_read_ivf_header_errmsg[7];
int32_t animvpx_read_ivf_header(buildvfs_kfd inhandle, animvpx_ivf_header_t *hdr);

typedef struct
{
    const char *errmsg;  // non-NULL if codec error? better always check...
    const char *errmsg_detail;  // may be NULL even if codec error

    uint16_t width, height;
    uint8_t *pic;  // lines of [Y U V 0], calloc'ed on init

    // VVV everything that follows should be considered private! VVV

    buildvfs_kfd inhandle;  // the kread() file handle

    // state of this struct:
    //  0: uninited (either not yet or already)
    //  1: codec init OK
    // -1: error while initing
    // -2: error while uniniting
    int32_t initstate;

    // decoder state:
    //  0: first time / begin
    //  1: have more frames
    //  2: reached EOF
    // -1: unspecified error
    // -2: decoder error
    int32_t decstate;

    uint32_t compbuflen;
    uint32_t compbufallocsiz;
    uint8_t *compbuf;  // compressed data buffer (one IVF/VP8 frame)

    vpx_codec_ctx_t codec;
    vpx_codec_iter_t iter;

    // statistics
    int32_t numframes;
    int32_t sumtimes[3];
    int32_t maxtimes[3];
} animvpx_codec_ctx;


int32_t animvpx_init_codec(const animvpx_ivf_header_t *info, buildvfs_kfd inhandle, animvpx_codec_ctx *codec);
int32_t animvpx_uninit_codec(animvpx_codec_ctx *codec);

extern const char *animvpx_nextpic_errmsg[8];
int32_t animvpx_nextpic(animvpx_codec_ctx *codec, uint8_t **pic);

void animvpx_setup_glstate(int32_t animvpx_flags);
void animvpx_restore_glstate(void);
int32_t animvpx_render_frame(animvpx_codec_ctx *codec, double animvpx_aspect);

void animvpx_print_stats(const animvpx_codec_ctx *codec);
#endif

static inline int32_t animvpx_check_header(const animvpx_ivf_header_t *hdr)
{
    if (Bmemcmp(hdr->magic,"DKIF",4))
        return 2;  // "not an IVF file"

    if (hdr->version != 0)
        return 3;  // "unrecognized IVF version"

    // fourcc is left as-is
    if (Bmemcmp(hdr->fourcc, "VP80", 4))
        return 4;  // "only VP8 supported"

    return 0;
}

#endif  // !defined ANIM_VPX_H
