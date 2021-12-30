
const int RF_ShadeInterpolate = 64;

//===========================================================================
//
//
//
//===========================================================================

vec4 ProcessTexel()
{
	int blendflags = int(uTextureAddColor.a);
	float fullbright = 0.0;
	vec4 color;
	float coordX = vTexCoord.x;
	float coordY = vTexCoord.y;
	vec2 newCoord;

	// z is the depth in view space, positive going into the screen
	float z = abs(pixelpos.w);

#ifdef NPOT_EMULATION
	// Coordinate adjustment for NPOT textures. It is somehow fitting that Build games exploited this texture wrapping quirk of the software rendering engine...
	if (uNpotEmulation.y != 0.0)
	{
		float period = floor(coordY / uNpotEmulation.y);
		coordX += uNpotEmulation.x * floor(mod(coordY, uNpotEmulation.y));
		coordY = period + mod(coordY, uNpotEmulation.y);
	}
#endif
	newCoord = vec2(coordX, coordY);

	color = texture(tex, newCoord);

	float visibility = max(uGlobVis * uLightFactor * z - ((blendflags & 16384) != 0? 0.5 : 0.0), 0.0);
	float numShades = float(uPalLightLevels & 255);
	float shade = (1.0 - uLightLevel) * (numShades);
	shade = clamp((shade + visibility), 0.0, numShades - 1.0);

	int palindex = int(color.r * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
	int shadeindex = int(floor(shade));
	float colorIndexF = texelFetch(texture3, ivec2(palindex, shadeindex), 0).r;
	int colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
	vec4 palettedColor = texelFetch(texture2, ivec2(colorIndex, 0), 0);

	if ((blendflags & 16384) != 0)
	{
		// Get the next shaded palette index for interpolation
		colorIndexF = texelFetch(texture3, ivec2(palindex, shadeindex+1), 0).r;
		colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
		vec4 palettedColorNext = texelFetch(texture2, ivec2(colorIndex, 0), 0);
		float shadeFrac = mod(shade, 1.0);
		palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac);
	}

	palettedColor.a = color.r == 0.0? 0.0 : 1.0;// 1.0-floor(color.r);
	color = palettedColor;

	if (color.a < uAlphaThreshold) discard;	// it's only here that we have the alpha value available to be able to perform the alpha test.
	return vec4(color.rgb, 1.0);
}
