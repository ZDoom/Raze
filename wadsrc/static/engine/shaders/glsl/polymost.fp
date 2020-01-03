#version 330

const int RF_ColorOnly = 1;
const int RF_UsePalette = 2;
const int RF_DetailMapping = 4;
const int RF_GlowMapping = 8;
const int RF_Brightmapping = 16;
const int RF_NPOTEmulation = 32;
const int RF_ShadeInterpolate = 64;
const int RF_FogDisabled = 128;

const int RF_HICTINT_Grayscale = 0x10000;
const int RF_HICTINT_Invert = 0x20000;
const int RF_HICTINT_Colorize = 0x40000;
const int RF_HICTINT_BLEND_Screen = 0x80000;
const int RF_HICTINT_BLEND_Overlay = 0x100000;
const int RF_HICTINT_BLEND_Hardlight = 0x200000;
const int RF_HICTINT_BLENDMASK = RF_HICTINT_BLEND_Screen | RF_HICTINT_BLEND_Overlay | RF_HICTINT_BLEND_Hardlight;


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

vec4 convertColor(vec4 color, int effect, vec3 tint)
{
#if 0

	if (effect & RF_HICTINT_Grayscale)
	{
		float g = grayscale(color);
		color = vec4(g, g, g, color.a);
	}

	if (effect & RF_HICTINT_Invert)
	{
		color = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b);
	}
	
	vec3 tcol = color.rgb * 255.0;	// * 255.0 to make it easier to reuse the integer math.
	tint *= 255.0;

	if (effect & RF_HICTINT_Colorize)
	{
		tcol.b = min(((tcol.b) * tint.r) / 64.0, 255.0);
		tcol.g = min(((tcol.g) * tint.g) / 64.0, 255.0);
		tcol.r = min(((tcol.r) * tint.b) / 64.0, 255.0);
	}

	switch (effect & RF_HICTINT_BLENDMASK)
	{
		case RF_HICTINT_BLEND_Screen:
			tcol.b = 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 256.0);
			tcol.g = 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 256.0);
			tcol.r = 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 256.0);
			break;
		case RF_HICTINT_BLEND_Overlay:
			tcol.b = tcol.b < 128.0? (tcol.b * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 128.0);
			tcol.g = tcol.g < 128.0? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 128.0);
			tcol.r = tcol.r < 128.0? (tcol.r * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 128.0);
			break;
		case RF_HICTINT_BLEND_Hardlight:
			tcol.b = tint.r < 128.0 ? (tcol.b * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - r)) / 128.0);
			tcol.g = tint.g < 128.0 ? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - g)) / 128.0);
			tcol.r = tint.b < 128.0 ? (tcol.r * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - b)) / 128.0);
			break;
	}
	color.rgb = tcol / 255.0;
#endif
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
		color = texture2D(s_texture, newCoord);

		// This was further down but it really should be done before applying any kind of depth fading, not afterward.
		vec4 detailColor = vec4(1.0);
		if ((u_flags & RF_DetailMapping) != 0)
		{
			detailColor = texture2D(s_detail, v_detailCoord.xy);
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
			
			fullbright = palettedColor.a;	// This only gets set for paletted rendering.
	   		palettedColor.a = c_one-floor(color.r);
	   		color = palettedColor;
			color.rgb *= detailColor.rgb;	// with all this palettizing, this can only be applied afterward, even though it is wrong to do it this way.
			if (fullbright == 0.0) color.rgb *= v_color.rgb; // Well, this is dead wrong but unavoidable. For colored fog it applies the light to the fog as well...
		}
		else
		{
			color.rgb *= detailColor.rgb;
			
			vec3 lightcolor = v_color.rgb;
			bool shadeIt = ((u_flags & RF_FogDisabled) == 0);
			// The lighting model here does not really allow more than a simple on/off brightmap because anything more complex inteferes with the shade ramp... :(
			if ((u_flags & RF_Brightmapping) != 0)
			{
				vec4 brightcolor = texture2D(s_brightmap, v_texCoord.xy);
				if (grayscale(brightcolor) > 0.5)
				{
					shadeIt = false;
				}
			}
			if (shadeIt)
			{
				color.rgb *= lightcolor;
				shade = clamp(shade * u_shadeDiv, 0.0, 1.0);	// u_shadeDiv is really 1/shadeDiv.
				// Apply the shade as a linear depth fade ramp.
				color.rgb = mix(color.rgb, u_fogColor.rgb, shade);
			}
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
		vec4 glowColor = texture2D(s_glow, v_texCoord.xy);
		color.rgb = mix(color.rgb, glowColor.rgb, glowColor.a);
	}
	
	color.rgb = pow(color.rgb, vec3(u_brightness));
	fragColor = color;
	fragFog = vec4(0.0, 0.0, 0.0, 1.0); // Does build have colored fog?
	vec3 normal = normalize(cross(dFdx(v_eyeCoordPosition.xyz), dFdy(v_eyeCoordPosition.xyz)));
	normal.x = -normal.x;
	normal.y = -normal.y;
	fragNormal = vec4(normal * 0.5 + 0.5, 1.0);
}
