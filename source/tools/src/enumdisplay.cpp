
#include "compat.h"

#define NEED_DDRAW_H
#include "windows_inc.h"

#define DEFAULT_OUTPUT_FILE "enumdisplay.txt"

HMODULE      hDDrawDLL = NULL;
LPDIRECTDRAW lpDD      = NULL;
FILE *output = NULL;

void usage(void)
{
    printf(
    "enumdisplay by Jonathon Fowler (jf@jonof.id.au)\n"
    "Options:\n"
    "   -h, -?, --help     This message\n"
    "   -o <filename>      Use different output file (default: "
        DEFAULT_OUTPUT_FILE ", use - for stdout)\n"
          );
}

void dumpdevmode(DEVMODE *devmode)
{
    fprintf(output, "\tdmFields has");
    if (devmode->dmFields & DM_PELSWIDTH) fprintf(output, " DM_PELSWIDTH");
    if (devmode->dmFields & DM_PELSHEIGHT) fprintf(output, " DM_PELSHEIGHT");
    if (devmode->dmFields & DM_BITSPERPEL) fprintf(output, " DM_BITSPERPEL");
    fprintf(output, "\n\tdmPelsWidth = %lu\n", devmode->dmPelsWidth);
    fprintf(output, "\tdmPelsHeight = %lu\n", devmode->dmPelsHeight);
    fprintf(output, "\tdmBitsPerPel = %lu\n", devmode->dmBitsPerPel);
}

HRESULT WINAPI ddenum(DDSURFACEDESC *ddsd, VOID *udata ATTRIBUTE((unused)))
{
    fprintf(output, "\tdwFlags has");
    if (ddsd->dwFlags & DDSD_WIDTH) fprintf(output, " DDSD_WIDTH");
    if (ddsd->dwFlags & DDSD_HEIGHT) fprintf(output, " DDSD_HEIGHT");
    if (ddsd->dwFlags & DDSD_PIXELFORMAT) fprintf(output, " DDSD_PIXELFORMAT");
    fprintf(output, "\n\tdwWidth = %lu\n", ddsd->dwWidth);
    fprintf(output, "\tdwHeight = %lu\n", ddsd->dwHeight);
    fprintf(output, "\tddpfPixelFormat.dwFlags has");
    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1) fprintf(output, " DDPF_PALETTEINDEXED1");
    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2) fprintf(output, " DDPF_PALETTEINDEXED2");
    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) fprintf(output, " DDPF_PALETTEINDEXED4");
    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) fprintf(output, " DDPF_PALETTEINDEXED8");
    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_RGB) fprintf(output, " DDPF_RGB");
    fprintf(output, "\n\tddpfPixelFormat.dwRGBBitCount = %lu\n", ddsd->ddpfPixelFormat.dwRGBBitCount);
    fprintf(output, "\n");

    return(DDENUMRET_OK);
}

typedef HRESULT (WINAPI *aDirectDrawCreateType)(GUID *, LPDIRECTDRAW *, IUnknown *);
typedef HRESULT (WINAPI *aDirectDrawEnumerateType)(LPDDENUMCALLBACK, LPVOID);

int InitDirectDraw(void)
{
    HRESULT result;
    aDirectDrawCreateType aDirectDrawCreate;
    aDirectDrawEnumerateType aDirectDrawEnumerate;

    hDDrawDLL = LoadLibrary("DDRAW.DLL");
    if (!hDDrawDLL) { fprintf(output, "Failed loading DDRAW.DLL\n"); return -1; }

    aDirectDrawEnumerate = (aDirectDrawEnumerateType)GetProcAddress(hDDrawDLL, "DirectDrawEnumerateA");
    if (!aDirectDrawEnumerate) { fprintf(output, "Error fetching DirectDrawEnumerate\n"); return -1; }

    aDirectDrawCreate = (aDirectDrawCreateType)GetProcAddress(hDDrawDLL, "DirectDrawCreate");
    if (!aDirectDrawCreate) { fprintf(output, "Error fetching DirectDrawCreate\n"); return -1; }

    result = aDirectDrawCreate(NULL, &lpDD, NULL);
    if (result != DD_OK) { fprintf(output, "DirectDrawCreate() failed (%ld)\n", result); return -1; }

    return 0;
}

void UninitDirectDraw(void)
{
    if (lpDD) IDirectDraw_Release(lpDD);
    lpDD = NULL;
    if (hDDrawDLL) FreeLibrary(hDDrawDLL);
    hDDrawDLL = NULL;
}

int main(int argc, char **argv)
{
    char const * outputfile = DEFAULT_OUTPUT_FILE;

    int i;

    DEVMODE devmode;
    HRESULT hresult;

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-?") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        } else if (!strcmp(argv[i], "-o")) {
            outputfile = argv[++i];
        }
    }

    if (!strcmp(outputfile, "-")) {
        output = fdopen(1, "wt");
        outputfile = NULL;
    } else {
        output = fopen(outputfile, "wt");
        if (!output) {
            fprintf(stderr, "enumdisplay: failed to open %s for output\n", outputfile);
            return 1;
        }
    }

    fprintf(output,
        "enumdisplay results\n"
        "\n"
        "Display settings:\n"
           );
    ZeroMemory(&devmode, sizeof(devmode));
    devmode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode)) {
        fprintf(output, "\tEnumDisplaySettings() FAILED!\n");
    } else {
        dumpdevmode(&devmode);
    }

    fprintf(output,
        "\n"
        "All modes from EnumDisplaySettings:\n"
           );
    ZeroMemory(&devmode, sizeof(devmode));
    devmode.dmSize = sizeof(DEVMODE);
    i = 0;
    while (EnumDisplaySettings(NULL, i, &devmode)) {
        dumpdevmode(&devmode);
        fprintf(output, "\n");
        ZeroMemory(&devmode, sizeof(devmode));
        devmode.dmSize = sizeof(DEVMODE);
        i++;
    }

    if (!InitDirectDraw()) {
        fprintf(output,
            "\n"
            "All modes from IDirectDraw::EnumDisplayModes:\n"
               );
        hresult = IDirectDraw_EnumDisplayModes(lpDD, 0, NULL, (LPVOID)0, ddenum);
        if (hresult != DD_OK) {
            fprintf(output, "\tIDirectDraw::EnumDisplayModes() FAILED! (%ld)\n", hresult);
        }
    }
    UninitDirectDraw();

    if (outputfile) fclose(output);

    return 0;
}

