#version 330

//s_texture points to an indexed color texture
uniform sampler2D s_texture;
//s_palswap is the palette swap texture where u is the color index and v is the shade
uniform sampler2D s_palswap;
//s_palette is the base palette texture where u is the color index
uniform sampler2D s_palette;

uniform sampler2D s_detail;
uniform sampler2D s_glow;

uniform vec2 u_palswapPos;
uniform vec2 u_palswapSize;

uniform vec2 u_clamp;

uniform float u_shade;
uniform float u_numShades;
uniform float u_visFactor;
uniform float u_fogEnabled;

uniform float u_useColorOnly;
uniform float u_usePalette;
uniform float u_npotEmulation;
uniform float u_npotEmulationFactor;
uniform float u_npotEmulationXOffset;
uniform float u_shadeInterpolate;
uniform float u_brightness;
uniform vec4 u_fog;
uniform vec4 u_fogColor;

uniform float u_useDetailMapping;
uniform float u_useGlowMapping;

uniform int u_tinteffect;
uniform vec3 u_tintcolor;

in vec4 v_color;
in float v_distance;
in vec4 v_texCoord;
in vec4 v_detailCoord;
in vec4 v_glowCoord;
in float v_fogCoord;

const float c_basepalScale = 255.0/256.0;
const float c_basepalOffset = 0.5/256.0;

const float c_zero = 0.0;
const float c_one = 1.0;
const float c_two = 2.0;
const vec4 c_vec4_one = vec4(c_one);
const float c_wrapThreshold = 0.9;

layout(location=0) out vec4 fragColor;

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

	if (effect & HICTINT_GRAYSCALE)
	{
		float g = grayscale(color);
		color = vec4(g, g, g, color.a);
	}

	if (effect & HICTINT_INVERT)
	{
		color = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b);
	}
	
	vec3 tcol = color.rgb * 255.0;	// * 255.0 to make it easier to reuse the integer math.
	tint *= 255.0;

	if (effect & HICTINT_COLORIZE)
	{
		tcol.b = min(((tcol.b) * tint.r) / 64.0, 255.0);
		tcol.g = min(((tcol.g) * tint.g) / 64.0, 255.0);
		tcol.r = min(((tcol.r) * tint.b) / 64.0, 255.0);
	}

	switch (effect & HICTINT_BLENDMASK)
	{
		case HICTINT_BLEND_SCREEN:
			tcol.b = 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 256.0);
			tcol.g = 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 256.0);
			tcol.r = 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 256.0);
			break;
		case HICTINT_BLEND_OVERLAY:
			tcol.b = tcol.b < 128.0? (tcol.b * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 128.0);
			tcol.g = tcol.g < 128.0? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 128.0);
			tcol.r = tcol.r < 128.0? (tcol.r * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 128.0);
			break;
		case HICTINT_BLEND_HARDLIGHT:
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
// Talk about all the wrong way of being 'efficient'... :(
//
//===========================================================================

void main()
{
	float fullbright = 0.0;
	vec4 color;
	if (u_useColorOnly == 0.0)
	{
		float coordX = v_texCoord.x;
		float coordY = v_texCoord.y;
		vec2 newCoord;
		
		// Coordinate adjustment for NPOT textures (something must have gone very wrong to make this necessary...)
		if (u_npotEmulation != 0.0)
		{
			float period = floor(coordY / u_npotEmulationFactor);
			coordX += u_npotEmulationXOffset * floor(mod(coordY, u_npotEmulationFactor));
			coordY = period + mod(coordY, u_npotEmulationFactor);
		}
		newCoord = vec2(coordX, coordY);
		if (u_clamp != 0.0) newCoord = clamp(newCoord.xy, 0.0, 1.0);

		// Paletted textures are stored in column major order rather than row major so coordinates need to be swapped here.
		color = texture2D(s_texture, newCoord);

		// This was further down but it really should be done before applying any kind of depth fading, not afterward.
		vec4 detailColor = vec4(1.0);
		if (u_useDetailMapping != 0.0)
		{
			detailColor = texture2D(s_detail, v_detailCoord.xy);
			detailColor = mix(vec4(1.0), 2.0 * detailColor, detailColor.a);
			// Application of this differs based on render mode because for paletted rendering with palettized shade tables it can only be done after processing the shade table. We only have a palette index before.
		}

		float visibility = max(u_visFactor * v_distance - 0.5 * u_shadeInterpolate, 0.0);
		float shade = clamp((u_shade + visibility), 0.0, u_numShades - 1.0);
		

		if (u_usePalette != 0.0)
		{
			// Get the shaded palette index
			float colorIndex = texture2D(s_palswap, u_palswapPos + u_palswapSize*vec2(color.r, floor(shade)/u_numShades)).r;
			colorIndex = c_basepalOffset + c_basepalScale*colorIndex;	// this is for compensating roundoff errors.
			vec4 palettedColor = texture2D(s_palette, vec2(colorIndex, c_zero));
			
			if (u_shadeInterpolate != 0.0)
			{
				// Get the next shaded palette index for interpolation
				colorIndex = texture2D(s_palswap, u_palswapPos+u_palswapSize*vec2(color.r, (floor(shade)+1.0)/u_numShades)).r;
				colorIndex = c_basepalOffset + c_basepalScale*colorIndex;	// this is for compensating roundoff errors.
				vec4 palettedColorNext = texture2D(s_palette, vec2(colorIndex, c_zero));
				float shadeFrac = mod(shade, 1.0);
				palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac);
			}
			
			fullbright = palettedColor.a;	// This only gets set for paletted rendering.
	   		palettedColor.a = c_one-floor(color.r);
	   		color = palettedColor.bgra;
			color.rgb *= detailColor.rgb;	// with all this palettizing, this can only be applied afterward, even though it is wrong to do it this way.
		}
		else
		{
			color.rgb *= detailColor.rgb;
			// todo: For True Color, calculate a shade value from the table and apply that to the color directly.
		}
		if (fullbright == 0.0) color.rgb *= v_color.rgb;
		color.a *= v_color.a;

		if (u_fogEnabled != 0.0)// the following would make sense if 'fullbright' could ever be true in non-paletted rendering: && (fullbright != 0.0 || u_fogColor.rgb != vec3(0.0) ))
		{
			float fogFactor;

			color.rgb *= detailColor.rgb;
			if (u_fog.z == 0) fogFactor = (u_fog.x-v_fogCoord)*u_fog.y;	// linear fog
	 		else fogFactor = exp2 (u_fog.z * v_fogCoord); 				// exponential fog

			fogFactor = clamp(fogFactor, 0.0, 1.0);
			color.rgb = mix(u_fogColor.rgb, color.rgb, fogFactor);
		}
	}
	else
	{
		// untextured rendering
		color = v_color;
	}

	if (u_useGlowMapping != 0.0 && u_useColorOnly == 0.0)
	{
		vec4 glowColor = texture2D(s_glow, v_glowCoord.xy);
		color.rgb = mix(color.rgb, glowColor.rgb, glowColor.a);
	}
	
	color.rgb = pow(color.rgb, vec3(u_brightness));
	fragColor = color;
}
