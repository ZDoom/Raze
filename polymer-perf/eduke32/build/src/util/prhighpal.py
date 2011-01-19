#!/usr/bin/python

import sys;

from numpy import array, zeros, ones, arange, uint32
from numpy import vstack, hstack, hsplit, dstack, dsplit

from PIL.Image import frombuffer


NBITS = 7;
RESIDBITS = 8-NBITS;
DIM = 1<<NBITS;

CONVFACT = 256;

###  target hues
green = .39;  # 11, 22
yellow = .17;  # 23
red = .04;  # 21


def genbasepal():
    "Generate base palette for highpalookup system. \
All other palettes must be based on this one."
    imint = zeros([DIM, DIM, DIM], 'uint32');
    for i in range(DIM):
        for j in range(DIM):
            for k in range(DIM):
                imint[i,j,k] = ((i<<16)|(k<<8)|j)<<RESIDBITS;

    imint = hstack(dsplit(imint, DIM));

    imbyte = zeros([DIM, DIM*DIM, 3], 'uint8');

    for i in range(3):
        imbyte[:,:,i] = (imint[:,:,0]>>(i*8))&255;

    return imbyte;


def getdispimg(im):
    "Get a reshaped version of the palette image IM suitable for viewing."
    # 2^NBITS x 2^NBITS*2^NBITS --> 2^(NBITS + NBITS/2) x 2^NBITS*2^(NBITS - NBITS/2)

    if (im.shape != (DIM, DIM*DIM, 3)):
        raise ValueError("image must have shape (DIM, DIM*DIM, 3)");

    dimfactor = 1<<(NBITS/2);
    return vstack(hsplit(im, dimfactor));


def getPILimg(im):
    sz = im.shape;
    return frombuffer("RGB", [sz[1], sz[0]], im, "raw", "RGB", 0, 1);


def saveimage(im, filename):
    getPILimg(im).save(filename);

def showpalimg(im):
    getPILimg(getdispimg(im)).show();


### utility functions

## port of Octave's rbg2hsv
def rgb2hsv(im):
    if (im.dtype=='uint8'):
        im = imitof(im);

    r, g, b = im[..., 0], im[..., 1], im[..., 2];
    s, v = im.min(2), im.max(2);
    dif = v-s;

    colored = (s != v);

    h = zeros(r.shape, im.dtype);

    # blue hue
    idx = ((v==b) & colored);
    h[idx] = 2./3 + 1./6*(r[idx]-g[idx])/dif[idx];

    # green hue
    idx = ((v==g) & colored);
    h[idx] = 1./3 + 1./6*(b[idx]-r[idx])/dif[idx];

    # red hue
    idx = ((v==r) & colored);
    h[idx] =        1./6*(g[idx]-b[idx])/dif[idx];
    h[idx] %= 1;

    s[~colored] = 0;
    s[colored] = 1.0 - s[colored]/v[colored];

    return dstack((h,s,v));


## port of Octave's hsv2rbg
def hsv2rgb(imh):
    imh[imh<0] = 0;
    imh[imh>1] = 1;

    h, s, v = imh[..., 0], imh[..., 1], imh[..., 2];

    rgb = v*(1.0-s);
    rgb = dstack((rgb, rgb, rgb));

    hue = dstack((h-2./3, h, h-1./3))%1;
    f = s*v;
    f = dstack((f, f, f));

    rgb += f * (6.0 * (hue < 1./6)*hue
                + ((hue >= 1./6) & (hue < 1./2))
                + ((hue >= 1./2) & (hue < 2./3))*(4.0 - 6.0*hue));

    return imftoi(rgb);


def imftoi(im):
    im = im.copy();
    if (im.dtype=='uint8'):
        return im
    im *= CONVFACT;
    im[im>255] = 255;
    return im.astype('uint8');

def imitof(im):
    if (im.dtype=='float32'):
        return im.copy();
    return im.astype('float32')/CONVFACT;


###
def genpal(basepal, basepalhsv, pal):
    "Generate a highpalookup image for palette number PAL. \
BASEPALHSV should the precomputed HSV representation of BASEPAL."

    if (basepal.dtype != 'uint8'):
        raise TypeError('BASEPAL should be uint8.');

    if (pal==0):
        return basepal;

    bph = basepalhsv;
    h,s,v = bph[..., 0], bph[..., 1], bph[..., 2];

    bluemask = (h>0.52) & (h<0.8) & (s>0.2);

    # all true mask will be used unless overridden
    mask = ones(h.shape, 'bool');


## PHRP r176 defs:
#
# tint { pal 1 red 100 green 120 blue 148 flags 1 }
# tint { pal 2 red 255 green  48 blue   0 flags 0 }
# tint { pal 4 red   0 green   0 blue   0 flags 0 }
# tint { pal 6 red 224 green 255 blue 112 flags 3 }
# tint { pal 7 red 172 green 157 blue 140 flags 0 }
# tint { pal 8 red 199 green 226 blue 113 flags 1 }
#
# bit 1: greyscale (max)
# bit 2: invert (255-x)
# colorization: min(int(C*C')/64, 255)

    if (pal in [1,2,4,6,7,8]):
        rgbf = { 1: [100, 120, 148, 1],
                 2: [255,  48,   0, 0],
                 4: [0,     0,   0, 0],
                 6: [224, 255, 112, 3],
                 7: [172, 157, 140, 0],
                 8: [199, 226, 113, 1]}

        newrgb = basepal.astype('uint32');

        flags = rgbf[pal][3]

        if (flags&1):  # greyscale
            newrgb = newrgb.max(2);
            newrgb = dstack((newrgb, newrgb, newrgb)).copy();

        if (flags&2):  # invert
            newrgb = 255-newrgb;

        # colorize
        for i in range(3):
            newrgb[:,:,i] *= rgbf[pal][i]
            newrgb[:,:,i] /= 255
        newrgb[newrgb>255] = 255

        return newrgb.astype('uint8');

# plagman:
#    if (pal==1):
#        h[:] = 0.66;

#    elif (pal==6):
#        h[:] = 0.33;
#        v = 1.0 - v;

    elif (pal==20):
        m1 = ((h>0.6) & (h<0.7));
        m2 = ((h>0.04) & (h<0.13));
        m3 = ((h>0.7) & (h<0.9));
        m4 = ((h>0.3) & (h<0.36));
        mask = m1 | m2 | m3 | m4;

        # blue to gray by removing all saturation
        s[m1] = 0.0;
        # orange and brown to blue
        h[m2] = 0.66;
        # purple and reddish to blue
        h[m3] = 0.66
        # green to blue
        h[m4] = 0.66;

# helixhorned:
    elif (pal==11):
        mask = bluemask;
        h[mask] = green;
        s[mask] += 0.1;

    elif (pal==12):
        mask = bluemask;
        h[mask] = 0.0;
        s[mask] = 0.0;

    elif (pal==13):
        mask = bluemask;
        h[mask] = 0.0;
        s[mask] = 0.0;
        v[mask] *= 0.7;

    elif (pal==16):
        mask = bluemask;
        s[mask] += 0.1;
        v[mask] -= 0.1;

    elif (pal==21):
        mask = bluemask;
        h[mask] = red;
        s[mask] += 0.3;

    elif (pal==23):
        mask = bluemask;
        h[mask] = yellow;
        s[mask] += 0.12;
        v[mask] *= 1.15;

    elif (pal==99):
        mask = bluemask;
        v[mask] = 0;

# user:
# ...

    else:
        raise ValueError("unknown pal {0}!".format(pal));

    # ---
    newrgb = hsv2rgb(dstack((h, s, v)));

    nmask = ~mask;

    r = mask*newrgb[:,:,0] + nmask*basepal[:,:,0];
    g = mask*newrgb[:,:,1] + nmask*basepal[:,:,1];
    b = mask*newrgb[:,:,2] + nmask*basepal[:,:,2];

    # PIL doesn't seem to like views/shallow copies
    return dstack((r, g, b)).copy();


## main
if (__name__ == "__main__"):

    argc = len(sys.argv);
    if (argc == 1):
        print "Usage: python prhighpal.py (palnums ...)"
        sys.exit();

    print "Generating base palette..."
    bp = genbasepal();
    bph = rgb2hsv(bp);

    for i in xrange(1, argc):
        palnum = int(sys.argv[i]);
        filename = "hipal{0}_gen.png".format(palnum);
        print "Generating palnum", palnum, "image ...";
        palimg = genpal(bp.copy(), bph.copy(), palnum);
        print "Writing", filename, "...";
        saveimage(palimg, filename);

    print "\nDEF code:\n"
    for i in xrange(1, argc):
        palnum = int(sys.argv[i]);
        if (palnum==0):
            continue;
        filename = "hipal{0}_gen.png".format(palnum);
        print "highpalookup { pal", palnum, "file", filename, "}";
