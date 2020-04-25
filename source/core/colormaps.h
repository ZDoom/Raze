#ifndef __RES_CMAP_H
#define __RES_CMAP_H

enum EColorManipulation
{
	CM_PLAIN2D = -2,			// regular 2D drawing.
	CM_INVALID = -1,
	CM_DEFAULT = 0,					// untranslated
	CM_FIRSTSPECIALCOLORMAP,		// first special fixed colormap
	CM_FIRSTSPECIALCOLORMAPFORCED = 0x08000000,	// first special fixed colormap, application forced (for 2D overlays)
};

#define CM_MAXCOLORMAP int(CM_FIRSTSPECIALCOLORMAP + SpecialColormaps.Size())
#define CM_MAXCOLORMAPFORCED int(CM_FIRSTSPECIALCOLORMAPFORCED + SpecialColormaps.Size())


#endif
