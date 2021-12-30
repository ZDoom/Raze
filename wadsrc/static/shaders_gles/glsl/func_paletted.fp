const int RF_ShadeInterpolate = 64;

const float pixleHalf = (1.0 / 256.0) / 2.0; // Half way through a pixel on 256 sized image
const float pixleIndexScale = 255.0 / 256.0;

vec4 getColor(float palindex, float shadeindex)
{
	float colorIndexF = texture2D(texture3, vec2(palindex, shadeindex)).a;
	vec4 palettedColor = texture2D(texture2, vec2((colorIndexF * pixleIndexScale) + pixleHalf, 0.5));

	return palettedColor;
}

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
	float period = floor(coordY / uNpotEmulation.y);
	coordX += uNpotEmulation.x * floor(mod(coordY, uNpotEmulation.y));
	coordY = period + mod(coordY, uNpotEmulation.y);
#endif

#endif

	newCoord = vec2(coordX, coordY);
	color = texture2D(tex, newCoord);

#if (DEF_PALETTE_INTERPOLATE == 1)
	float visibility = max(uGlobVis * uLightFactor * z - 0.5, 0.0);
#else
	float visibility = max(uGlobVis * uLightFactor * z, 0.0);
#endif


	float numShades = float(uPalLightLevels);
	float shade = (1.0 - uLightLevel) * (numShades);
	shade = clamp((shade + visibility), 0.0, numShades - 1.0);

	float palindex = (color.a * pixleIndexScale) + pixleHalf;
	float shadeindex = (shade / (numShades)) + pixleHalf;

	vec4 palettedColor = getColor(palindex, shadeindex);


#if (DEF_PALETTE_INTERPOLATE == 1)
	// Get the next shaded palette index for interpolation
	shadeindex = ((shade + 1.0) / (numShades));
	shadeindex = clamp(shadeindex, 0.0, 1.0);

	vec4 palettedColorNext = getColor(palindex, shadeindex);

	float shadeFrac = mod(shade, 1.0);
	palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac);
#endif


	//palettedColor.a = color.a == 0.0? 0.0 : 1.0;
	// Replaces above line without branch
	palettedColor.a = floor(color.a + 0.999);

	color = palettedColor;

	//if (color.a < uAlphaThreshold) discard;	// it's only here that we have the alpha value available to be able to perform the alpha test.
	// This replaces the above line to avoid the branch and the discard.. Seems to look the same but could be unforeseen issues.
	float alpha = step(uAlphaThreshold, color.a);

	return vec4(color.rgb, alpha);
}

