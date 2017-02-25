
#include "compat.h"
#ifndef USE_OPENGL
# define USE_OPENGL
#endif
#define ANIMVPX_STANDALONE
#include "animvpx.h"


static void print_info(const char *prefix, const animvpx_ivf_header_t *hdr)
{
    printf("%s%d x %d, %d frames @ %d frames / %d seconds (%.3f fps%s)\n", prefix,
           hdr->width, hdr->height, hdr->numframes, hdr->fpsnumer, hdr->fpsdenom,
           (hdr->fpsdenom==0 ? 0 : (double)hdr->fpsnumer/hdr->fpsdenom),
           hdr->fpsnumer>=1000 ? " --> 30 fps" : "");
}


int main(int argc, char **argv)
{
    int fd, dowrite, err;
    animvpx_ivf_header_t hdr;

    union { uint16_t i; char c[2]; } u;
    u.c[0] = 1;
    u.c[1] = 0;
    if (u.i != 1)
    {
        fprintf(stderr, "This program is only for little-endian machines.\n");
        return 255;
    }

    if (argc == 2 && argv[1][0]=='-')
        goto usage;

    if (argc != 2 && argc != 4 && argc != 5)
    {
usage:
        fprintf(stderr, "Usage: %s <file.ivf> [<fpsnumerator> <fpsdenominator> [-force]]\n"
                " Without -force, <fpsnumerator> must be < 1000.\n"
                " If <fpsnumerator> is >= 1000, the actual frame rate\n"
                " is set to 30 fps on playback.\n",
                argv[0]);
        return 1;
    }

    dowrite = (argc >= 4);

    fd = open(argv[1], dowrite ? O_RDWR : O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Could't open \"%s\" for %s: %s\n", argv[1],
                dowrite ? "reading/writing":"reading", strerror(errno));
        return 2;
    }

    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
    {
        fprintf(stderr, "Couldn't read IVF header: %s\n", strerror(errno));
        return 3;
    }

    err = animvpx_check_header(&hdr);
    if (err)
    {
        fprintf(stderr, "Header check failed with code %d (not an IVF file?)\n", err);
        return 4;
    }

    if (!dowrite)
    {
        print_info("", &hdr);
    }
    else
    {
        unsigned long numer = strtoul(argv[2], NULL, 10);
        unsigned long denom = strtoul(argv[3], NULL, 10);
        uint32_t numer32=numer, denom32=denom;
        const int NUMER_OFS = 16;  //offsetof(animvpx_ivf_header_t, fpsnumer);

        if (denom == 0)
        {
            fprintf(stderr, "FPS denominator must not be zero!\n");
            return 4;
        }

        if (numer >= 1000 && (argc!=5 || strcmp(argv[4], "-force")))
        {
            fprintf(stderr, "FPS numerator must be < 1000, or -force must be passed as 5th arg.\n");
            return 5;
        }

        if (numer32 != numer || denom32 != denom)
        {
            fprintf(stderr, "Out of range number passed.\n");
            return 6;
        }

        print_info("Old: ", &hdr);
        hdr.fpsnumer = numer32;
        hdr.fpsdenom = denom32;
        print_info("New: ", &hdr);

        if (lseek(fd, NUMER_OFS, SEEK_SET) != NUMER_OFS)
        {
            fprintf(stderr, "lseek failed: %s\n", strerror(errno));
            return 7;
        }

        err = 0;
        err |= (write(fd, &numer32, 4) != 4);
        err |= (write(fd, &denom32, 4) != 4);

        if (err)
        {
            fprintf(stderr, "Warning: data not fully written.\n");
            return 8;
        }
    }

    return 0;
}
