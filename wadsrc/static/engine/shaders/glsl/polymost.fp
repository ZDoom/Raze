#version 330

const int RF_ColorOnly = 1;
const int RF_UsePalette = 2;
const int RF_DetailMapping = 4;
const int RF_GlowMapping = 8;
const int RF_Brightmapping = 16;
const int RF_NPOTEmulation = 32;
const int RF_ShadeInterpolate = 64;
const int RF_FogDisabled = 128;
const int RF_MapFog = 256;

const int RF_TINT_Grayscale = 0x1;
const int RF_TINT_Invert = 0x2;
const int RF_TINT_Colorize = 0x4;
const int RF_TINT_BLEND_Screen = 64;
const int RF_TINT_BLEND_Overlay = 128;
const int RF_TINT_BLEND_Hardlight = 192;
const int RF_TINT_BLENDMASK = RF_TINT_BLEND_Screen | RF_TINT_BLEND_Overlay | RF_TINT_BLEND_Hardlight;


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
uniform float u_numShades;
uniform float u_shadeDiv;
uniform float u_visFactor;
uniform int u_flags;
uniform float u_alphaThreshold;

uniform vec4 u_tintOverlay, u_tintModulate;
uniform int u_tintFlags;
uniform vec4 u_fullscreenTint;

uniform float u_npotEmulationFactor;
uniform float u_npotEmulationXOffset;
uniform float u_brightness;
uniform vec4 u_fogColor;
uniform vec3 u_tintcolor;
uniform vec3 u_tintmodulate;

in vec4 v_color;
in float v_distance;
in vec4 v_texCoord;
in vec4 v_detailCoord;
in float v_fogCoord;
in vec4 v_eyeCoordPosition;

const float c_basepalScale = 255.0/256.0;
const float c_basepalOffset = 0.5/256.0;

const float c_zero = 0.0;
const float c_one = 1.0;
const float c_two = 2.0;
const vec4 c_vec4_one = vec4(c_one);
const float c_wrapThreshold = 0.9;

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
// Hightile tinting code. (hictinting[dapalnum]) This can be done inside the shader 
// to avoid costly texture duplication (but needs a more modern GLSL than 1.10.)
//
//===========================================================================

vec4 convertColor(vec4 color)
{
	int effect = u_tintFlags;
	if ((effect & RF_TINT_Grayscale) != 0)
	{
		float g = grayscale(color);
		color = vec4(g, g, g, color.a);
	}

	if ((effect & RF_TINT_Invert) != 0)
	{
		color = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, color.a);
	}
	
	vec3 tcol = color.rgb * 255.0;	// * 255.0 to make it easier to reuse the integer math.

	// Much of this looks quite broken by design. Why is this effectively multplied by 4 if the flag is set...? :(
	if ((effect & RF_TINT_Colorize) != 0)
	{
		tcol.r = min(((tcol.b) * u_tintModulate.r)* 4, 255.0);
		tcol.g = min(((tcol.g) * u_tintModulate.g)* 4, 255.0);
		tcol.b = min(((tcol.r) * u_tintModulate.b)* 4, 255.0);
	}
	else
	{
		tcol.r = min(((tcol.b) * u_tintModulate.r), 255.0);
		tcol.g = min(((tcol.g) * u_tintModulate.g), 255.0);
		tcol.b = min(((tcol.r) * u_tintModulate.b), 255.0);
	}

	vec4 ov = u_tintOverlay * 255.0;
	switch (effect & RF_TINT_BLENDMASK)
	{
		case RF_TINT_BLEND_Screen:
			tcol.r = 255.0 - (((255.0 - tcol.r) * (255.0 - ov.r)) / 256.0);
			tcol.g = 255.0 - (((255.0 - tcol.g) * (255.0 - ov.g)) / 256.0);
			tcol.b = 255.0 - (((255.0 - tcol.b) * (255.0 - ov.b)) / 256.0);
			break;
		case RF_TINT_BLEND_Overlay:
			tcol.r = tcol.b < 128.0? (tcol.r * ov.r) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - ov.r)) / 128.0);
			tcol.g = tcol.g < 128.0? (tcol.g * ov.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - ov.g)) / 128.0);
			tcol.b = tcol.r < 128.0? (tcol.b * ov.b) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - ov.b)) / 128.0);
			break;
		case RF_TINT_BLEND_Hardlight:
			tcol.r = ov.r < 128.0 ? (tcol.r * ov.r) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - ov.r)) / 128.0);
			tcol.g = ov.g < 128.0 ? (tcol.g * ov.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - ov.g)) / 128.0);
			tcol.b = ov.b < 128.0 ? (tcol.b * ov.b) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - ov.b)) / 128.0);
			break;
	}
	color.rgb = tcol / 255.0;
	return color;
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
			detailColor = texture(s_detail, v_detailCoord.xy);
			detailColor = mix(vec4(1.0), 2.0 * detailColor, detailColor.a);
			// Application of this differs based on render mode because for paletted rendering with palettized shade tables it can only be done after processing the shade table. We only have a palette index before.
		}

		float visibility = max(u_visFactor * v_distance - ((u_flags & RF_ShadeInterpolate) != 0.0? 0.5 : 0.0), 0.0);
		float shade = clamp((u_shade + visibility), 0.0, u_numShades - 1.0);
		

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
			
	   		palettedColor.a = color.r == 0.0? 0.0 : 1.0;// c_one-floor(color.r);
	   		color = palettedColor;
			color.rgb *= detailColor.rgb;	// with all this palettizing, this can only be applied afterward, even though it is wrong to do it this way.
			color.rgb *= v_color.rgb; // Well, this is dead wrong but unavoidable. For colored fog it applies the light to the fog as well...
		}
		else
		{
			color.rgb *= detailColor.rgb;
			if (u_tintFlags != -1) color = convertColor(color);
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
			float fogfactor = 0.55 + 0.3 * exp2 ((-5.0 / 1024.0)*v_distance); 		
			color.rgb = vec3(0.6*(1.0-fogfactor)) + color.rgb * fogfactor;// mix(vec3(0.6), color.rgb, fogfactor);
		}
		if (color.a < u_alphaThreshold) discard;	// it's only here that we have the alpha value available to be able to perform the alpha test.
		
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
	
	color.rgb = pow(color.rgb, vec3(u_brightness));
	color.rgb *= u_fullscreenTint.rgb;	// must be the last thing to be done.
	fragColor = color;
	fragFog = vec4(0.0, 0.0, 0.0, 1.0); // Does build have colored fog?
	vec3 normal = normalize(cross(dFdx(v_eyeCoordPosition.xyz), dFdy(v_eyeCoordPosition.xyz)));
	normal.x = -normal.x;
	normal.y = -normal.y;
	fragNormal = vec4(normal * 0.5 + 0.5, 1.0);
}
