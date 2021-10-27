const int RF_ShadeInterpolate = 64;

vec4 ProcessTexel()
{
	float fullbright = 0.0;
	vec4 color;
	float coordX = vTexCoord.x;
	float coordY = vTexCoord.y;
	vec2 newCoord;
	
	// z is the depth in view space, positive going into the screen
	float z = abs(pixelpos.w);
	
#ifdef NPOT_EMULATION
	// Coordinate adjustment for NPOT textures. It is somehow fitting that Build games exploited this texture wrapping quirk of the software rendering engine...
#if (DEF_NPOT_EMULATION == 1)
	{
		float period = floor(coordY / uNpotEmulation.y);
		coordX += uNpotEmulation.x * floor(mod(coordY, uNpotEmulation.y));
		coordY = period + mod(coordY, uNpotEmulation.y);
	}
#endif

#endif
	newCoord = vec2(coordX, coordY);

	color = texture(tex, newCoord);

#if ((DEF_BLEND_FLAGS & 16384) != 0)
	float visibility = max(uGlobVis * uLightFactor * z - 0.5, 0.0);
#else
	float visibility = max(uGlobVis * uLightFactor * z, 0.0);
#endif
	
	float numShades = float(uPalLightLevels & 255);
	float shade = (1.0 - uLightLevel) * (numShades);
	shade = clamp((shade + visibility), 0.0, numShades - 1.0);
	
	int palindex = int(color.r * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
	int shadeindex = int(floor(shade));
	float colorIndexF = texelFetch(texture3, ivec2(palindex, shadeindex), 0).r;
	int colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
	vec4 palettedColor = texelFetch(texture2, ivec2(colorIndex, 0), 0);

#if ((DEF_BLEND_FLAGS & 16384) != 0)	
	{
		// Get the next shaded palette index for interpolation
		colorIndexF = texelFetch(texture3, ivec2(palindex, shadeindex+1), 0).r;
		colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
		vec4 palettedColorNext = texelFetch(texture2, ivec2(colorIndex, 0), 0);
		float shadeFrac = mod(shade, 1.0);
		palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac);
	}
#endif

	//palettedColor.a = color.r == 0.0? 0.0 : 1.0;// 1.0-floor(color.r);

	// Replaces above line without branch
	palettedColor.a = floor(color.r + 0.999);

	color = palettedColor;
	
	//if (color.a < uAlphaThreshold) discard;	// it's only here that we have the alpha value available to be able to perform the alpha test.

	// This replaces the above line to avoid the branch and the discard.. Seems to look the same but could be unforeseen issues.
	float alpha = step(uAlphaThreshold, color.a);

	return vec4(color.rgb, alpha);
}

