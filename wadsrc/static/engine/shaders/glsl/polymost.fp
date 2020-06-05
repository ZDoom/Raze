const int RF_ColorOnly = 1;
const int RF_UsePalette = 2;
const int RF_DetailMapping = 4;
const int RF_GlowMapping = 8;
const int RF_Brightmapping = 16;
const int RF_NPOTEmulation = 32;
const int RF_ShadeInterpolate = 64;
const int RF_FogDisabled = 128;
const int RF_MapFog = 256;

//s_texture points to an indexed color texture
uniform sampler2D s_texture;
//s_palswap is the palette swap texture where u is the color index and v is the shade
uniform sampler2D s_palswap;
//s_palette is the base palette texture where u is the color index
uniform sampler2D s_palette;

uniform sampler2D s_detail;
uniform sampler2D s_glow;
uniform sampler2D s_brightmap;

uniform float u_shade;
uniform float u_shadeDiv;
uniform float u_visFactor;
uniform int u_flags;

uniform float u_npotEmulationFactor;
uniform float u_npotEmulationXOffset;
uniform float u_brightness;
uniform vec4 u_fogColor;

in vec4 v_color;
in float v_distance;
in vec4 v_texCoord;
in vec4 v_detailCoord;
in float v_fogCoord;
in vec4 v_eyeCoordPosition;
in vec4 v_worldPosition;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec4 fragFog;
layout(location=2) out vec4 fragNormal;

//===========================================================================
//
// Color to grayscale
//
//===========================================================================

float grayscale(vec4 color)
{
	return dot(color.rgb, vec3(0.3, 0.56, 0.14));
}

//===========================================================================
//
// Desaturate a color
//
//===========================================================================

vec4 dodesaturate(vec4 texel, float factor)
{
	if (factor != 0.0)
	{
		float gray = grayscale(texel);
		return mix (texel, vec4(gray,gray,gray,texel.a), factor);
	}
	else
	{
		return texel;
	}
}

//===========================================================================
//
// Texture tinting code originally from JFDuke but with a few more options
//
//===========================================================================

const int Tex_Blend_Alpha = 1;
const int Tex_Blend_Screen = 2;
const int Tex_Blend_Overlay = 3;
const int Tex_Blend_Hardlight = 4;
 
 vec4 ApplyTextureManipulation(vec4 texel, int blendflags)
 {
	// Step 1: desaturate according to the material's desaturation factor. 
	texel = dodesaturate(texel, uTextureModulateColor.a);
	
	// Step 2: Invert if requested
	if ((blendflags & 8) != 0)
	{
		texel.rgb = vec3(1.0 - texel.r, 1.0 - texel.g, 1.0 - texel.b);
	}
	
	// Step 3: Apply additive color
	texel.rgb += uTextureAddColor.rgb;
	
	// Step 4: Colorization, including gradient if set.
	texel.rgb *= uTextureModulateColor.rgb;
	
	// Before applying the blend the value needs to be clamped to [0..1] range.
	texel.rgb = clamp(texel.rgb, 0.0, 1.0);
	
	// Step 5: Apply a blend. This may just be a translucent overlay or one of the blend modes present in current Build engines.
	if ((blendflags & 7) != 0)
	{
		vec3 tcol = texel.rgb * 255.0;	// * 255.0 to make it easier to reuse the integer math.
		vec4 tint = uTextureBlendColor * 255.0;

		switch (blendflags & 7)
		{
			default:
				tcol.b = tcol.b * (1.0 - uTextureBlendColor.a) + tint.b * uTextureBlendColor.a;
				tcol.g = tcol.g * (1.0 - uTextureBlendColor.a) + tint.g * uTextureBlendColor.a;
				tcol.r = tcol.r * (1.0 - uTextureBlendColor.a) + tint.r * uTextureBlendColor.a;
				break;
			// The following 3 are taken 1:1 from the Build engine
			case Tex_Blend_Screen:
				tcol.b = 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 256.0);
				tcol.g = 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 256.0);
				tcol.r = 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 256.0);
				break;
			case Tex_Blend_Overlay:
				tcol.b = tcol.b < 128.0? (tcol.b * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - tint.b)) / 128.0);
				tcol.g = tcol.g < 128.0? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 128.0);
				tcol.r = tcol.r < 128.0? (tcol.r * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - tint.r)) / 128.0);
				break;
			case Tex_Blend_Hardlight:
				tcol.b = tint.b < 128.0 ? (tcol.b * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - tint.b)) / 128.0);
				tcol.g = tint.g < 128.0 ? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 128.0);
				tcol.r = tint.r < 128.0 ? (tcol.r * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - tint.r)) / 128.0);
				break;
		}
		texel.rgb = tcol / 255.0;
	}
	return texel;
}


//===========================================================================
//
//
//
//===========================================================================

void main()
{
	float fullbright = 0.0;
	vec4 color;
	if ((u_flags & RF_ColorOnly) == 0)
	{
		float coordX = v_texCoord.x;
		float coordY = v_texCoord.y;
		vec2 newCoord;
		
		// Coordinate adjustment for NPOT textures (something must have gone very wrong to make this necessary...)
		if ((u_flags & RF_NPOTEmulation) != 0)
		{
			float period = floor(coordY / u_npotEmulationFactor);
			coordX += u_npotEmulationXOffset * floor(mod(coordY, u_npotEmulationFactor));
			coordY = period + mod(coordY, u_npotEmulationFactor);
		}
		newCoord = vec2(coordX, coordY);

		// Paletted textures are stored in column major order rather than row major so coordinates need to be swapped here.
		color = texture(s_texture, newCoord);

		// This was further down but it really should be done before applying any kind of depth fading, not afterward.
		vec4 detailColor = vec4(1.0);
		if ((u_flags & RF_DetailMapping) != 0)
		{
			detailColor = texture(s_detail, newCoord * uDetailParms.xy) * uDetailParms.z;
			detailColor = mix(vec4(1.0), 2.0 * detailColor, detailColor.a);
			// Application of this differs based on render mode because for paletted rendering with palettized shade tables it can only be done after processing the shade table. We only have a palette index before.
		}

		float visibility = max(uGlobVis * u_visFactor * v_distance - ((u_flags & RF_ShadeInterpolate) != 0.0? 0.5 : 0.0), 0.0);
		float numShades = float(uPalLightLevels & 255);
		float shade = clamp((u_shade + visibility), 0.0, numShades - 1.0);
		

		if ((u_flags & RF_UsePalette) != 0)
		{
			int palindex = int(color.r * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
			int shadeindex = int(floor(shade));
			float colorIndexF = texelFetch(s_palswap, ivec2(palindex, shadeindex), 0).r;
			int colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
			vec4 palettedColor = texelFetch(s_palette, ivec2(colorIndex, 0), 0);
			
			if ((u_flags & RF_ShadeInterpolate) != 0)
			{
				// Get the next shaded palette index for interpolation
				colorIndexF = texelFetch(s_palswap, ivec2(palindex, shadeindex+1), 0).r;
				colorIndex = int(colorIndexF * 255.0 + 0.1); // The 0.1 is for roundoff error compensation.
				vec4 palettedColorNext = texelFetch(s_palette, ivec2(colorIndex, 0), 0);
				float shadeFrac = mod(shade, 1.0);
				palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac);
			}
			
	   		palettedColor.a = color.r == 0.0? 0.0 : 1.0;// 1.0-floor(color.r);
	   		color = palettedColor;
			color.rgb *= detailColor.rgb;	// with all this palettizing, this can only be applied afterward, even though it is wrong to do it this way.
			color.rgb *= v_color.rgb; // Well, this is dead wrong but unavoidable. For colored fog it applies the light to the fog as well...
		}
		else
		{
			color.rgb *= detailColor.rgb;
			
			// Apply the texture modification colors.
			int blendflags = int(uTextureAddColor.a);	// this alpha is unused otherwise
			if (blendflags != 0)	
			{
				// only apply the texture manipulation if it contains something.
				color = ApplyTextureManipulation(color, blendflags);
			}
			
			if ((u_flags & RF_FogDisabled) == 0)
			{
				shade = clamp(shade * u_shadeDiv, 0.0, 1.0);	// u_shadeDiv is really 1/shadeDiv.
				vec3 lightcolor = v_color.rgb * (1.0 - shade);

				if ((u_flags & RF_Brightmapping) != 0)
				{
					lightcolor = clamp(lightcolor + texture(s_brightmap, v_texCoord.xy).rgb, 0.0, 1.0);
				}
				color.rgb *= lightcolor;
				if ((u_flags & RF_MapFog) == 0) color.rgb += u_fogColor.rgb * shade;
			}
			else color.rgb *= v_color.rgb;
		}
		if ((u_flags & RF_MapFog) != 0) // fog hack for RRRA E2L1. Needs to be done better, this is gross, but still preferable to the broken original implementation.
		{
			float fogfactor = 0.55 + 0.3 * exp2 (-5.0*v_fogCoord); 		
			color.rgb = vec3(0.6*(1.0-fogfactor)) + color.rgb * fogfactor;// mix(vec3(0.6), color.rgb, fogfactor);
		}
		if (color.a < uAlphaThreshold) discard;	// it's only here that we have the alpha value available to be able to perform the alpha test.
		
		color.a *= v_color.a;
	}
	else
	{
		// untextured rendering
		color = v_color;
	}

	if ((u_flags & (RF_ColorOnly|RF_GlowMapping)) == RF_GlowMapping)
	{
		vec4 glowColor = texture(s_glow, v_texCoord.xy);
		color.rgb = mix(color.rgb, glowColor.rgb, glowColor.a);
	}
	
	/*
	int ix = int (v_worldPosition.x);
	int iy = int (v_worldPosition.z);
	int iz = int (v_worldPosition.y);
	if ((ix & 64) == 1) color.r = 0;
	if ((iy & 64) == 1) color.g = 0;
	if ((iz & 64) == 1) color.b = 0;
	*/
	
	color.rgb = pow(color.rgb, vec3(u_brightness));
	fragColor = color;
	fragFog = vec4(0.0, 0.0, 0.0, 1.0); // Does build have colored fog?
	vec3 normal = normalize(cross(dFdx(v_eyeCoordPosition.xyz), dFdy(v_eyeCoordPosition.xyz)));
	normal.x = -normal.x;
	normal.y = -normal.y;
	fragNormal = vec4(normal * 0.5 + 0.5, 1.0);
}
