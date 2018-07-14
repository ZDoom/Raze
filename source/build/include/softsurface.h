/*
 * softsurface.h
 *  An 8-bit rendering surface that can quickly upscale and blit 8-bit paletted buffers to an external 32-bit screen buffer.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#ifndef SOFTSURFACE_H_
#define SOFTSURFACE_H_

#include "compat.h"

// Initialize the softsurface with the Software renderer's buffer resolution.
// If the Software renderer's resolution and the actual resolution don't match,
//  softsurface will still render at the full size of the screen.
// If a surface already exists, softsurface_destroy() will be automatically called before re-initializing.
// Returns whether or not the softsurface could be successfully initialized.
bool softsurface_initialize(vec2_t bufferResolution,
                            vec2_t destBufferResolution);

// Destroy an existing surface.
void softsurface_destroy();

// Sets the palette to contain the RGBA byte buffer pointed to by pPalette.
// destRedMask/destGreenMask/destBlueMask mask the bits that represent each colour component in the destination buffer's pixel format.
// If the surface is not initialized, the function returns immediately.
void softsurface_setPalette(void* pPalette,
                            uint32_t destRedMask,
                            uint32_t destGreenMask,
                            uint32_t destBlueMask);

// Returns a pointer to the start of the surface's 8-bit pixel buffer
// Returns NULL if the surface is not initialized.
uint8_t* softsurface_getBuffer();

// Returns the resolution of the surface's buffer
vec2_t softsurface_getBufferResolution();

// Returns the resolution of the destination buffer
vec2_t softsurface_getDestinationBufferResolution();

// Blit the surface's pixel buffer to the destination buffer using the palette set with softsurface_setPalette().
// If the surface is not initialized, the function returns immediately.
void softsurface_blitBuffer(uint32_t* destBuffer,
                            uint32_t destBpp);

#endif /* SOFTSURFACE_H_ */
