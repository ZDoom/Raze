#include "compat.h"

struct hsv {
    float h, s, v;
};

struct rgb {
    float r, g, b;
};

struct gradient {
    int start, len, chompends;
    struct hsv startcolour, endcolour;
};

float min2(float x, float y);
float max2(float x, float y);
void convertHSVtoRGB(struct hsv *hsv, struct rgb *rgb);
int showusage(void);
int readscript(char *fn);

struct gradient ramps[256];
int nramps = 0;

int main(int argc, char **argv)
{
    struct hsv palette[256], lerpstep, lerped;
    struct rgb rgbidx;
    unsigned char rgbout[3];
    int idx, step, rampnum;
    FILE* fh;
    char const * outfile = "palette.dat";

    memset(palette,0,sizeof(palette));

    if (argc < 2) return showusage();
    if (readscript(argv[1])) return 1;
    if (argc >= 3) outfile = argv[2];

    for (rampnum = 0; rampnum < nramps; rampnum++) {
        idx = ramps[rampnum].start;
        step = ramps[rampnum].len;
        if (ramps[rampnum].chompends & 1) step++;
        if (ramps[rampnum].chompends & 2) step++;
        lerpstep.h = (ramps[rampnum].endcolour.h - ramps[rampnum].startcolour.h) / (float)step;
        lerpstep.s = (ramps[rampnum].endcolour.s - ramps[rampnum].startcolour.s) / (float)step;
        lerpstep.v = (ramps[rampnum].endcolour.v - ramps[rampnum].startcolour.v) / (float)step;
        lerped = ramps[rampnum].startcolour;
        if (ramps[rampnum].chompends & 1) {
            step--;
            lerped.h += lerpstep.h;
            lerped.s += lerpstep.s;
            lerped.v += lerpstep.v;
        }
        if (ramps[rampnum].chompends & 2) step--;

        for (; step > 0; step--,idx++) {
            palette[idx].h = lerped.h;
            palette[idx].s = lerped.s;
            palette[idx].v = lerped.v;
            lerped.h += lerpstep.h;
            lerped.s += lerpstep.s;
            lerped.v += lerpstep.v;
        }
    }

    fh = fopen(outfile,"wb");
    if (!fh) return 1;

    for (idx=0; idx<256; idx++) {
        convertHSVtoRGB(&palette[idx], &rgbidx);
        //printf("Index %d: r=%g g=%g b=%g\n",idx,rgbidx.r,rgbidx.g,rgbidx.b);
        rgbout[0] = (unsigned char)min2(255,max2(0,(int)(rgbidx.r * 255.0))) >> 2;
        rgbout[1] = (unsigned char)min2(255,max2(0,(int)(rgbidx.g * 255.0))) >> 2;
        rgbout[2] = (unsigned char)min2(255,max2(0,(int)(rgbidx.b * 255.0))) >> 2;
        fwrite(rgbout,3,1,fh);
    }

    fclose(fh);

    return 0;
}

float min2(float x, float y)
{
    return x < y ? x : y;
}

float max2(float x, float y)
{
    return x > y ? x : y;
}

// http://www.cs.rit.edu/~ncs/color/t_convert.html
void convertHSVtoRGB(struct hsv *hsv, struct rgb *rgb)
{
    int i;
    float f, p, q, t;
    if( hsv->s == 0 ) {
        // achromatic (grey)
        rgb->r = rgb->g = rgb->b = hsv->v;
        return;
    }
    hsv->h /= 60;            // sector 0 to 5
    i = floor( hsv->h );
    f = hsv->h - i;            // factorial part of h
    p = hsv->v * ( 1 - hsv->s );
    q = hsv->v * ( 1 - hsv->s * f );
    t = hsv->v * ( 1 - hsv->s * ( 1 - f ) );
    switch( i ) {
        case 0:
            rgb->r = hsv->v;
            rgb->g = t;
            rgb->b = p;
            break;
        case 1:
            rgb->r = q;
            rgb->g = hsv->v;
            rgb->b = p;
            break;
        case 2:
            rgb->r = p;
            rgb->g = hsv->v;
            rgb->b = t;
            break;
        case 3:
            rgb->r = p;
            rgb->g = q;
            rgb->b = hsv->v;
            break;
        case 4:
            rgb->r = t;
            rgb->g = p;
            rgb->b = hsv->v;
            break;
        default:        // case 5:
            rgb->r = hsv->v;
            rgb->g = p;
            rgb->b = q;
            break;
    }
}

int showusage(void)
{
    puts("mkpalette <palettescript.txt> [outputfile]");
    puts("If outputfile is not given, palette.dat is assumed");

    puts("\nPalette script format:\n"
    "  A line beginning with # is a comment, otherwise each line contains none\n"
    "values separated by spaces defining the gradient:\n"
    "\n"
    "  startindex rangesize skip starthue startsat startval endhue endsat endval\n"
    "\n"
    "Any text after the end of a gradient description is ignored, so may use it\n"
    "to describe the colour.\n"
    "\n"
    "* 'startindex' specifies the first palette index to write to\n"
    "* 'rangesize' specifies the length of the gradient\n"
    "* 'skip' specifies whether the first and/or last elements of the range should\n"
    "  be ignored and with 'rangesize' elements interpolated between. This is so\n"
    "  you can have a gradient starting at (potentially) pure black and ending at\n"
    "  (potentially) pure white but without wasting a palette index on those colours\n"
    "  if they already exist, eg. in a grey ramp.\n"
    "  Legal values are 0 (no skipping), 1 (skip start), 2 (skip end),\n"
    "  or 3 (skip start and end).\n"
    "* 'starthue', 'startsat', 'startval' are integers specifying the beginning\n"
    "  colour in Hue-Saturation-Value format.\n"
    "  'starthue' should be in the range 0-360 indicating a degrees value\n"
    "  'startsat' should be in the range 0-100 indicating a saturation percentage\n"
    "  'startval' should be in the range 0-100 indicating an intensity percentage\n"
    "* 'endhue', 'endsat', 'endval' specify the ending colour."
    );
    return 0;
}

int readscript(char *fn)
{
    int start, len, skip, shue, ssat, sval, ehue, esat, eval;
    FILE *fp;
    char line[1024];

    fp = fopen(fn,"rt");
    if (!fp) {
        puts("Error opening palette script");
        return 1;
    }

    while (fgets(line,sizeof(line),fp)) {
        {
            // test for comment
            char *p = line;
            while (*p && (*p == ' ' || *p == '\t')) p++;
            if (*p == '#') continue;
        }

        if (sscanf(line, "%d %d %d %d %d %d %d %d %d", &start,&len,&skip,&shue,&ssat,&sval,&ehue,&esat,&eval) < 9)
            continue;

        if (start < 0 || start > 255) {
            printf("start index of %d is out of range 0-255\n", start);
            continue;
        }
        if (len < 1 || len > 255) {
            printf("length %d is out of range 1-255\n", len);
            continue;
        }
        if (skip != (skip&3)) {
            printf("skip value of %d is out of range 0-3\n", skip);
            continue;
        }
        if (shue < 0 || shue > 360) {
            printf("start hue %d is out of range 0-360\n", shue);
            continue;
        }
        if (ssat < 0 || ssat > 100) {
            printf("start saturation %d is out of range 0-100\n", ssat);
            continue;
        }
        if (sval < 0 || sval > 100) {
            printf("start value %d is out of range 0-100\n", sval);
            continue;
        }
        if (ehue < 0 || ehue > 360) {
            printf("end hue %d is out of range 0-360\n", shue);
            continue;
        }
        if (esat < 0 || esat > 100) {
            printf("end saturation %d is out of range 0-100\n", ssat);
            continue;
        }
        if (eval < 0 || eval > 100) {
            printf("end value %d is out of range 0-100\n", sval);
            continue;
        }

        ramps[nramps].start = start;
        ramps[nramps].len = len;
        ramps[nramps].chompends = skip;
        ramps[nramps].startcolour.h = (float)shue;
        ramps[nramps].startcolour.s = (float)ssat / 100.0;
        ramps[nramps].startcolour.v = (float)sval / 100.0;
        ramps[nramps].endcolour.h = (float)ehue;
        ramps[nramps].endcolour.s = (float)esat / 100.0;
        ramps[nramps].endcolour.v = (float)eval / 100.0;
        nramps++;
    }

    fclose(fp);
    return 0;
}
