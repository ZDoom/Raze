#!/usr/bin/python

def makehighpalookuppixel(pal, rgb):
    hsv = list(colorsys.rgb_to_hsv(*rgb))
    if pal == 0:
        # no-op, pass through
        return rgb
    # frozen, blue light
    elif pal == 1:
        # make everything blueish
        hsv[0] = 0.66
        return list(colorsys.hsv_to_rgb(*hsv))
    # nightvision
    elif pal == 6:
        # make everything green and reverse brightness
        hsv[0] = 0.33
        hsv[2] = 1.0 - hsv[2]
        #hsv[1] = 0.5
        return list(colorsys.hsv_to_rgb(*hsv))
    # pal 20
    elif pal == 20:
        # blue to gray by removing all saturation
        if (hsv[0] > 0.6 and hsv[0] < 0.7):
            hsv[1] = 0
        # orange and brown to blue
        if (hsv[0] > 0.04 and hsv[0] < 0.13):
            hsv[0] = 0.66
        # purple and reddish to blue
        if (hsv[0] > 0.7 and hsv[0] < 0.9):
            hsv[0] = 0.66
        # green to blue
        if (hsv[0] > 0.30 and hsv[0] < 0.36):
            hsv[0] = 0.66
        return list(colorsys.hsv_to_rgb(*hsv))
    else:
        print "unknown pal!"
        sys.exit()

import colorsys
import sys
import struct

if (len(sys.argv) != 3):
    print "Usage: python highpalookupmaker.py palnum outfile"
    sys.exit()

# bit depth per dimension
xbits = 7
ybits = 7
zbits = 7

xdim = 1 << xbits
ydim = 1 << ybits
zdim = 1 << zbits

palnum = int(sys.argv[1])

pixels = []
pixelcount = xdim * ydim * zdim
curpix = 0.0

fo = open(sys.argv[2], "w")

# throw in a TGA header in there, this way they'll be able to directly edit it if they feel like it
fo.write(struct.pack("=BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 16384, 128, 32, 0))

print "Creating highpalookup map %s for palette %d with depth %d:%d:%d..." % (sys.argv[2], palnum, xbits, ybits, zbits)

for k in range(zdim):
    for j in range(ydim):
        for i in range(xdim):
            rgb = [float(i) / (xdim - 1), float(j) / (ydim - 1), float(k) / (zdim - 1)]
            rgb = makehighpalookuppixel(palnum, rgb)
            # save as BGRA as that's what TGA uses
            pixels.append(struct.pack('BBBB', int(rgb[2] * 255), int(rgb[1] * 255), int(rgb[0] * 255), 255))
            curpix += 1
            if (curpix % 128 == 0):
                print "\r%f%% done." % (curpix * 100 / pixelcount),

fo.writelines(pixels)
fo.close()

print "\n"
