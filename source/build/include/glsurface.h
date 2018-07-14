/*
 * glsurface.h
 *  A 32-bit rendering surface that can quickly blit 8-bit paletted buffers implemented in OpenGL.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#ifndef GLSURFACE_H_
#define GLSURFACE_H_

#include "compat.h"

// Initialize the glsurface with the Software renderer's buffer resolution.
// If the Software renderer's resolution and the actual resolution don't match,
//  glsurface will still render at the full size of the screen.
// If a surface already exists, glsurface_destroy() will be automatically called before re-initializing.
// Returns whether or not the glsurface could be successfully initialized.
bool glsurface_initialize(vec2_t bufferResolution);

// Destroy an existing surface.
void glsurface_destroy();

// Sets the palette to contain the RGBA byte buffer pointed to by pPalette.
// If the surface is not initialized, the function returns immediately.
void glsurface_setPalette(void* pPalette);

// Returns a pointer to the start of the surface's pixel buffer
// Returns NULL if the surface is not initialized.
void* glsurface_getBuffer();

// Returns the resolution of the surface's buffer
vec2_t glsurface_getBufferResolution();

// Blit the surface's pixel buffer to the screen using the palette set with glsurface_setPalette().
// Renders as soon as the data has been uploaded.
// If the surface is not initialized, the function returns immediately.
void glsurface_blitBuffer();

#endif /* GLSURFACE_H_ */
